#version 460
#extension GL_NV_ray_tracing : require

struct ShadowRayPayload 
{
	float lightIntensity;
};

layout(location = 1) rayPayloadInNV ShadowRayPayload shadowRayPayload;

void main()
{
	shadowRayPayload.lightIntensity = 1.0f;
}