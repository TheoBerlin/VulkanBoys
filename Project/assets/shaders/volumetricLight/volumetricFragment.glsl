#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.1415926535897932384626433832795

layout (push_constant) uniform PushConstants
{
	vec2 viewportExtent;
	uint raymarchSteps;
} g_PushConstants;

layout (binding = 1) uniform VolumetricPointLight
{
	mat4 worldMatrix;

	// Light data
	vec4 lightPosition, lightColor;
	float lightRadius, lightScatterAmount;

	// Determines the portion of forward scattered light
	float particleG;
} g_Light;

layout (binding = 2) uniform CameraMatrices
{
	mat4 Projection;
	mat4 View;
	mat4 InvView;
	mat4 InvProjection;
	vec4 Position;
} g_Camera;

layout(binding = 3) uniform sampler2D u_DepthBuffer;

layout(location = 0) in vec3 in_WorldPosition;

layout(location = 0) out vec4 outColor;

vec3 getSphereFrontIntersection(vec3 rayDir, vec3 sphereCenter, float radius)
{
	// in_WorldPosition is on the sphere's rear edge, and the ray facing outwards, away from the camera
	vec3 toCenter = sphereCenter - in_WorldPosition;

	// t: coefficient that brings ray to its closest point to the sphere center
	float t = dot(rayDir, toCenter);

	return in_WorldPosition + rayDir * (t * 2.0);
}

// Either a point on the rear edge of the sphere or on geometry intersecting the sphere
vec3 getRayDestination()
{
	vec2 texCoords = gl_FragCoord.xy / g_PushConstants.viewportExtent;
	float depth = min(gl_FragCoord.z, texture(u_DepthBuffer, texCoords).r);

	vec4 clipPos = vec4(texCoords * 2.0 - vec2(1.0), depth, 1.0);
	vec4 viewPos = g_Camera.InvProjection * clipPos;
	viewPos /= viewPos.w;

	vec4 worldPos = g_Camera.InvView * viewPos;
	return worldPos.xyz;
}

vec3 getRayOrigin(vec3 rayDir)
{
	vec3 camPos = g_Camera.Position.xyz;
	vec3 lightPos = g_Light.lightPosition.xyz;
	float lightRadius = g_Light.lightRadius;

	if (length(camPos - lightPos) < lightRadius) {
		return camPos;
	} else {
		return getSphereFrontIntersection(rayDir, lightPos, lightRadius);
	}
}

float phaseFunction(vec3 rayDir, vec3 posToLightDir)
{
	float g = g_Light.particleG;
	float viewAngle = dot(rayDir, posToLightDir);

	// Henyey-Greenstein phase function - an approximation of Mie scattering
	float numerator = (1 - g) * (1 - g);
	float denominator = 4 * PI * pow(1 + g * g - 2.0 * g * cos(viewAngle), 1.5);

	return numerator / denominator;
}

vec3 calculateLight(vec3 position, vec3 rayDir)
{
	vec3 posToLight = g_Light.lightPosition.xyz - position;
	float lightDistance = length(posToLight);

	// Diffuse light
	float attenuation = 1.0 / (lightDistance * lightDistance);

	// The normal is pointing right at the light
	vec3 diffuse = g_Light.lightColor.xyz * attenuation;

	return diffuse * phaseFunction(rayDir, posToLight / lightDistance) * g_Light.lightScatterAmount;
}

void main()
{
	vec3 rayDestination = getRayDestination();
    vec3 rayDir = normalize(rayDestination - g_Camera.Position.xyz);
	vec3 rayOrigin = getRayOrigin(rayDir);

	// Ray-march section
	vec3 raymarchStep = rayDir * length(rayDestination - rayOrigin) / g_PushConstants.raymarchSteps;

	vec3 light = vec3(0);
	vec3 worldPos = rayOrigin;

	for (uint stepNr = 0; stepNr < g_PushConstants.raymarchSteps; stepNr++) {
		light += calculateLight(worldPos, rayDir);

		worldPos += raymarchStep;
	}

	light /= g_PushConstants.raymarchSteps;
    outColor = vec4(light, 1.0);
}
