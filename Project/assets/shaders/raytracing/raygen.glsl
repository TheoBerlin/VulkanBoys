#version 460
#extension GL_NV_ray_tracing : require

#define EPSILON 0.001f

layout(binding = 0, set = 0, rgba8) uniform image2D u_ResultImage;
layout(binding = 1, set = 0) uniform CameraProperties 
{
	mat4 viewInverse;
	mat4 projInverse;
} u_Cam;
layout(binding = 2, set = 0) uniform accelerationStructureNV u_TopLevelAS;
layout(binding = 3, set = 0) uniform sampler2D u_Albedo_AO;
layout(binding = 4, set = 0) uniform sampler2D u_Normal_Roughness;
layout(binding = 5, set = 0) uniform sampler2D u_Depth;

layout (constant_id = 2) const int MAX_POINT_LIGHTS = 4;

struct PointLight
{
	vec4 Color;
	vec4 Position;
};

layout (binding = 16) uniform LightBuffer
{
	PointLight lights[MAX_POINT_LIGHTS];
} u_Lights;

struct RayPayload 
{
	vec3 color;
	uint recursion;
};

struct ShadowRayPayload
{
	float occluderFactor;
};

layout(location = 0) rayPayloadNV RayPayload rayPayload;
layout(location = 1) rayPayloadNV ShadowRayPayload shadowRayPayload;

vec3 calculateHitPosition(vec2 uvCoords, float z)
{
	vec2 xy = uvCoords * 2.0f - 1.0f;

	vec4 clipSpacePosition = vec4(xy, z, 1.0f);
	vec4 viewSpacePosition = u_Cam.projInverse * clipSpacePosition;
	vec4 homogenousPosition = u_Cam.viewInverse * viewSpacePosition;

	return homogenousPosition.xyz / homogenousPosition.w;
}

vec3 calculateDirection(vec2 uvCoords, vec3 hitPosition, vec3 normal)
{
	vec4 u_CameraOrigin = u_Cam.viewInverse * vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec3 origDirection = normalize(hitPosition - u_CameraOrigin.xyz);

	return reflect(origDirection, normal);
}

void main() 
{
	const vec2 pixelCenter = vec2(gl_LaunchIDNV.xy) + vec2(0.5f);
	vec2 uvCoords = (pixelCenter / vec2(gl_LaunchSizeNV.xy));

	vec4 sampledAlbedoAO = texture(u_Albedo_AO, uvCoords);
	vec4 sampledNormalRoughness = texture(u_Normal_Roughness, uvCoords);
	vec3 normal = normalize(sampledNormalRoughness.xyz);
	float sampledDepth = texture(u_Depth, uvCoords).r;

	if (dot(sampledNormalRoughness.xyz, sampledNormalRoughness.xyz) < 0.5f)
	{
		imageStore(u_ResultImage, ivec2(gl_LaunchIDNV.xy), vec4(1.0f, 0.0f, 1.0f, 1.0f));
		return;
	}

	vec3 hitPosition = calculateHitPosition(uvCoords, sampledDepth);
	vec3 origin = hitPosition + sampledNormalRoughness.xyz * EPSILON;

	vec3 direction = calculateDirection(uvCoords, origin, sampledNormalRoughness.xyz);
	

	uint rayFlags = gl_RayFlagsOpaqueNV;
	uint cullMask = 0xff;
	float tmin = 0.001f;
	float tmax = 10000.0f;

	//Reflection/Refraction
	rayPayload.color = vec3(0.0f);
	rayPayload.recursion = 0;
	traceNV(u_TopLevelAS, rayFlags, cullMask, 0, 0, 0, origin, tmin, direction, tmax, 0);

	//Direct Light
	vec3 finalLightColor = vec3(0.0f);

	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		vec3 lightPosition 	= u_Lights.lights[i].Position.xyz;
		vec3 lightColor 	= u_Lights.lights[i].Color.rgb;

		vec3 lightDirection = (lightPosition - origin);
		float distance 		= length(lightDirection);
		float attenuation 	= 1.0f / (distance * distance);

		lightDirection 	= normalize(lightDirection);

		traceNV(u_TopLevelAS, rayFlags, cullMask, 1, 0, 1, origin, tmin, lightDirection, tmax, 1);

		float tempFactor = 0.8f;
		finalLightColor += attenuation * lightColor * (1.0f - shadowRayPayload.occluderFactor * tempFactor);
	}

	const float reflectiveness = 1.0f - sampledNormalRoughness.a;
	vec3 finalColor = (reflectiveness * rayPayload.color + (1.0f - reflectiveness) * sampledAlbedoAO.rgb) * finalLightColor;

	imageStore(u_ResultImage, ivec2(gl_LaunchIDNV.xy), vec4(finalColor, 1.0f));
}