#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.1415926535897932384626433832795

layout(location = 0) in vec3 in_WorldPosition;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform PushConstants
{
	mat4 worldMatrix;

	// Light data
	vec4 lightPosition, lightColor;
	float lightRadius, lightScatterAmount;
	// Determines the portion of forward scattered light
	float particleG;

	uint raymarchSteps;
} g_PushConstants;

layout (binding = 1) uniform CameraMatrices
{
	mat4 Projection;
	mat4 View;
	mat4 InvView;
	mat4 InvProjection;
	vec4 Position;
} g_Camera;

layout(binding = 2) uniform sampler2D u_DepthBuffer;
// sampler3D?
//layout(binding = 5) uniform sampler3D u_LightTexture;

float sphereRearIntersect(vec3 rayOrigin, vec3 rayDir, vec3 sphereCenter, float radius)
{
	// Assuming that the ray's origin is on the sphere's edge, facing inwards
	vec3 toCenter = sphereCenter - rayOrigin;

	// t: coefficient that brings ray to its closest point to the sphere center
	float t = dot(rayDir, toCenter);

	return t * 2.0;
	// Based on algorithm presented in Real-Time Rendering, Third Edition
	// if (s < 0.0) {
	// 	return false;
	// }

	// float lSquared = dot(l, l);

	// // s < 0 means there is an intersection behind the camera
	// // lSquared > r^2 means the camera is outside the sphere
	// if (s < 0.0 && lSquared > radius * radius) {
	// 	return false;
	// }

	// d: distance between sphere center and L

	// L: rayOrigin + tc * rayDir
}

float phaseFunction(vec3 rayDir, vec3 posToLightDir)
{
	float g = g_PushConstants.particleG;
	float viewAngle = dot(rayDir, posToLightDir);

	// Henyey-Greenstein phase function - an approximation of Mie scattering
	float numerator = (1 - g) * (1 - g);
	float denominator = 4 * PI * pow(1 + g * g - 2.0 * g * cos(viewAngle), 1.5);

	return numerator / denominator;
}

vec3 calculateLight(vec3 position, vec3 rayDir)
{
	vec3 posToLight = g_PushConstants.lightPosition.xyz - position;
	float lightDistance = length(posToLight);

	// Diffuse light
	float attenuation = 1.0 / (lightDistance * lightDistance);

	// The normal is pointing right at the light
	vec3 diffuse = g_PushConstants.lightColor.xyz * attenuation;

	return diffuse * phaseFunction(rayDir, posToLight / lightDistance) * g_PushConstants.lightScatterAmount;

	//vec3 normal = posToLight / lightDistance;
}

void main()
{
    float depthVal = texture(u_DepthBuffer, gl_FragCoord.xy).r;

	if (gl_FragCoord.z > depthVal) {
		// The light is occluded
		discard;
		return;
	}

    vec3 worldPosition = in_WorldPosition;

	vec3 rayOrigin = g_Camera.Position.xyz;
    vec3 rayDir = normalize(worldPosition - rayOrigin);

	// The world position is on the 'front' of the sphere, find the rear intersection point
	float intersect = sphereRearIntersect(rayOrigin, rayDir, g_PushConstants.lightPosition.xyz, g_PushConstants.lightRadius);

	// Which is intersected first, the rear of the sphere or geometry?
	vec4 intersectPos = g_Camera.Projection * g_Camera.View * vec4(rayOrigin + rayDir * intersect, 0.0);
	float maxIntersectDistance = min(intersectPos.z / intersectPos.w, depthVal);

	// Ray-march section
	float raymarchStepLength = maxIntersectDistance / g_PushConstants.raymarchSteps;

	vec3 light = vec3(0);

	for (uint raymarchStep = 0; raymarchStep < g_PushConstants.raymarchSteps; raymarchStep++) {
		light += calculateLight(worldPosition, rayDir);

		worldPosition += rayDir;
	}

	light /= g_PushConstants.raymarchSteps;
    outColor = vec4(light, 1.0);
}
