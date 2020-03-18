#version 460
#extension GL_NV_ray_tracing : require

struct ShadowRayPayload 
{
	float OccluderFactor;
};

layout(location = 1) rayPayloadInNV ShadowRayPayload shadowRayPayload;

void main()
{
	shadowRayPayload.OccluderFactor = 0.0f;
}