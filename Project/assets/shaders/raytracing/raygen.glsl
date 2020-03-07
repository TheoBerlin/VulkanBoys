#version 460
#extension GL_NV_ray_tracing : require

layout(binding = 0, set = 0, rgba8) uniform image2D resultImage;
layout(binding = 1, set = 0) uniform CameraProperties 
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 lightPos;
} cam;
layout(binding = 2, set = 0) uniform accelerationStructureNV topLevelAS;
layout(binding = 3, set = 0) uniform sampler2D g_albedo;
layout(binding = 4, set = 0) uniform sampler2D g_normal;
layout(binding = 5, set = 0) uniform sampler2D g_depth;


struct RayPayload 
{
	vec3 color;
	uint recursion;
};

layout(location = 0) rayPayloadNV RayPayload rayPayload;

vec3 calculateHitPosition(vec2 uvCoords, vec2 d)
{
	float z = texture(g_depth, uvCoords).r * 2.0f - 1.0f;

	vec4 clipSpacePosition = vec4(d, z, 1.0f);
	vec4 viewSpacePosition = cam.projInverse * clipSpacePosition;
	viewSpacePosition /= viewSpacePosition.w;

	return (cam.viewInverse * viewSpacePosition).xyz;
}

vec3 calculateDirection(vec2 uvCoords, vec3 hitPosition)
{
	vec4 cameraOrigin = cam.viewInverse * vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec3 origDirection = normalize(hitPosition - cameraOrigin.xyz);
	vec3 normal = texture(g_normal, uvCoords).xyz;

	return reflect(origDirection, normal);
}

void main() 
{
	const vec2 pixelCenter = vec2(gl_LaunchIDNV.xy) + vec2(0.5f);
	vec2 uvCoords = (pixelCenter / vec2(gl_LaunchSizeNV.xy));
	vec2 d = uvCoords * 2.0f - 1.0f;

	vec3 hitPosition = calculateHitPosition(uvCoords, d);
	vec3 direction = calculateDirection(uvCoords, hitPosition);
	vec4 albedoColor = texture(g_albedo, uvCoords);

	uint rayFlags = gl_RayFlagsOpaqueNV;
	uint cullMask = 0xff;
	float tmin = 0.001f;
	float tmax = 10000.0f;

	rayPayload.color = vec3(0.0f);
	rayPayload.recursion = 0;
	//rayPayload.occluderFactor = 0.0f;
	traceNV(topLevelAS, rayFlags, cullMask, 0, 0, 0, hitPosition, tmin, direction, tmax, 0);

	const float reflectiveness = 0.5f;
	vec3 finalColor = reflectiveness * rayPayload.color + (1.0f - reflectiveness) * albedoColor.rgb;

	imageStore(resultImage, ivec2(gl_LaunchIDNV.xy), vec4(finalColor, 1.0f));
}