#version 460
#extension GL_NV_ray_tracing : require

struct ShadowRayPayload 
{
	float occluderFactor;
};

layout(location = 1) rayPayloadInNV ShadowRayPayload shadowRayPayload;

void main()
{
	shadowRayPayload.occluderFactor = 0.0f;
}