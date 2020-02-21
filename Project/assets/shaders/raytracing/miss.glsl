#version 460
#extension GL_NV_ray_tracing : require

struct RayPayload 
{
	vec3 color;
	uint recursion;
};

layout(location = 0) rayPayloadInNV RayPayload rayPayload;

void main()
{
	// View-independent background gradient to simulate a basic sky background
	const vec3 gradientStart = vec3(0.5f, 0.6f, 1.0f);
	const vec3 gradientEnd = vec3(1.0f);
	vec3 unitDir = normalize(gl_WorldRayDirectionNV);
	float t = 0.5f * (unitDir.y + 1.0f);
	rayPayload.color = (1.0f-t) * gradientStart + t * gradientEnd;
	rayPayload.recursion = rayPayload.recursion + 1;
}