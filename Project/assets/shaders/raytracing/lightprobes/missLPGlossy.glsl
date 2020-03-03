#version 460
#extension GL_NV_ray_tracing : require

struct RayPayload 
{
	vec3 color;
	float distance;
	uint recursion;
};

layout (constant_id = 0) const float PROBE_STEP_X = 1.0f;
layout (constant_id = 1) const float PROBE_STEP_Y = 1.0f;
layout (constant_id = 2) const float PROBE_STEP_Z = 1.0f;
layout (constant_id = 3) const uint NUM_PROBES_PER_DIMENSION = 2;

layout(location = 0) rayPayloadInNV RayPayload rayPayload;

void main()
{
	// View-independent background gradient to simulate a basic sky background
	const vec3 gradientStart = vec3(0.5f, 0.6f, 1.0f);
	const vec3 gradientEnd = vec3(1.0f);
	vec3 unitDir = normalize(gl_WorldRayDirectionNV);
	float t = 0.5f * (unitDir.y + 1.0f);
	rayPayload.color = (1.0f-t) * gradientStart + t * gradientEnd;
	
	vec3 worldSize = vec3(PROBE_STEP_X, PROBE_STEP_Y, PROBE_STEP_Z) * (NUM_PROBES_PER_DIMENSION - 1);

	rayPayload.distance = length(worldSize);
	rayPayload.recursion = rayPayload.recursion + 1;
}