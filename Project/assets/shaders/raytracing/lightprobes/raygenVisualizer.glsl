#version 460
#extension GL_NV_ray_tracing : require

layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;
layout(binding = 1, set = 0, rgba8) uniform image2D image;
layout(binding = 2, set = 0) uniform CameraProperties 
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 lightPos;
} cam;


struct RayPayload 
{
	vec3 color;
	uint recursion;
};

layout(location = 0) rayPayloadNV RayPayload rayPayload;

void main() 
{
	const vec2 pixelCenter = vec2(gl_LaunchIDNV.xy) + vec2(0.5f);
	const vec2 inUV = pixelCenter / vec2(gl_LaunchSizeNV.xy);
	vec2 d = inUV * 2.0f - 1.0f;

	vec4 origin = cam.viewInverse * vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 target = cam.projInverse * vec4(d.x, d.y, 1.0f, 1.0f) ;
	vec4 direction = cam.viewInverse * vec4(normalize(target.xyz / target.w), 0.0f);

	uint rayFlags = gl_RayFlagsOpaqueNV;
	uint cullMask = 0x40;
	float tmin = 0.001f;
	float tmax = 10000.0f;

	rayPayload.color = vec3(0.0f);
	rayPayload.recursion = 0;
	//rayPayload.occluderFactor = 0.0f;
	traceNV(topLevelAS, rayFlags, cullMask, 0, 0, 0, origin.xyz, tmin, direction.xyz, tmax, 0);
	imageStore(image, ivec2(gl_LaunchIDNV.xy), vec4(rayPayload.color, 1.0));
}