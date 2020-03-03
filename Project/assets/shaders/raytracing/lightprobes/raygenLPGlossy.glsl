#version 460
#extension GL_NV_ray_tracing : require

#include "helpers.glsl"

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
layout (constant_id = 4) const uint SAMPLES_PER_PROBE_DIMENSION = 8;
layout (constant_id = 5) const uint SAMPLES_PER_PROBE = 64;

layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;
layout(binding = 9, set = 0, r11f_g11f_b10f) uniform image2D glossySourceRadianceAtlas;
layout(binding = 10, set = 0, r32f) uniform image2D glossySourceDepthAtlas;

layout(location = 0) rayPayloadNV RayPayload rayPayload;

void main() 
{
	vec3 worldSize = vec3(PROBE_STEP_X, PROBE_STEP_Y, PROBE_STEP_Z) * (NUM_PROBES_PER_DIMENSION - 1);
	vec3 lightProbeCenter = (gl_LaunchIDNV / vec3(NUM_PROBES_PER_DIMENSION - 1)) * worldSize - worldSize / 2.0f;
	
	float sqrt5 = sqrt(5.0f);

	uint lightProbeIndex = (gl_LaunchIDNV.x + NUM_PROBES_PER_DIMENSION * (gl_LaunchIDNV.y + NUM_PROBES_PER_DIMENSION * gl_LaunchIDNV.z));

	ivec2 realTextureOffset = ivec2((SAMPLES_PER_PROBE_DIMENSION + 2) * lightProbeIndex, 0);
	ivec2 adjustedTextureOffset = realTextureOffset + ivec2(1);

	const uint SAMPLES_PER_DIMENSION = uint(sqrt(SAMPLES_PER_PROBE));

	vec4 leftDiffuseIrradianceEdge[SAMPLES_PER_PROBE_DIMENSION];
	vec4 topDiffuseIrradianceEdge[SAMPLES_PER_PROBE_DIMENSION];
	vec4 rightDiffuseIrradianceEdge[SAMPLES_PER_PROBE_DIMENSION];
	vec4 bottomDiffuseIrradianceEdge[SAMPLES_PER_PROBE_DIMENSION];

	vec4 leftDepthEdge[SAMPLES_PER_PROBE_DIMENSION];
	vec4 topDepthEdge[SAMPLES_PER_PROBE_DIMENSION];
	vec4 rightDepthEdge[SAMPLES_PER_PROBE_DIMENSION];
	vec4 bottomDepthEdge[SAMPLES_PER_PROBE_DIMENSION];

	for (uint n = 0; n < SAMPLES_PER_PROBE; n++)
	{
		//vec3 rayDirection = createSampleDirection(n, gr);
		ivec2 texelCoords = ivec2(n % SAMPLES_PER_DIMENSION, n / SAMPLES_PER_DIMENSION);
		vec3 rayDirection = oct_to_float32x3(((vec2(texelCoords) + vec2(0.5f)) / float(SAMPLES_PER_DIMENSION)) * 2.0f - 1.0f);

		uint rayFlags = gl_RayFlagsOpaqueNV;
		uint cullMask = 0x80;
		float tmin = 0.001f;
		float tmax = 10000.0f;

		rayPayload.color = vec3(0.0f);
		rayPayload.distance = tmax;
		rayPayload.recursion = 0;
		traceNV(topLevelAS, rayFlags, cullMask, 0, 0, 0, lightProbeCenter, tmin, rayDirection, tmax, 0);
		
		vec3 radiance = rayPayload.color;
		float depth = rayPayload.distance;

		imageStore(glossySourceRadianceAtlas, texelCoords + adjustedTextureOffset, vec4(radiance, 1.0f));
		imageStore(glossySourceDepthAtlas, texelCoords + adjustedTextureOffset, vec4(depth, 1.0, 1.0f, 1.0f));

		if (texelCoords.x == 0)
		{
			leftDiffuseIrradianceEdge[texelCoords.y] = vec4(radiance, 1.0f);
			leftDepthEdge[texelCoords.y] = vec4(depth, 1.0, 1.0f, 1.0f);
		}
		else if (texelCoords.x == SAMPLES_PER_DIMENSION - 1)
		{
			rightDiffuseIrradianceEdge[texelCoords.y] = vec4(radiance, 1.0f);
			rightDepthEdge[texelCoords.y] = vec4(depth, 1.0, 1.0f, 1.0f);
		}

		if (texelCoords.y == 0)
		{
			bottomDiffuseIrradianceEdge[texelCoords.x] = vec4(radiance, 1.0f);
			bottomDepthEdge[texelCoords.x] = vec4(depth, 1.0, 1.0f, 1.0f);
		}
		else if (texelCoords.y == SAMPLES_PER_DIMENSION - 1)
		{
			topDiffuseIrradianceEdge[texelCoords.x] = vec4(radiance, 1.0f);
			topDepthEdge[texelCoords.x] = vec4(depth, 1.0, 1.0f, 1.0f);
		}
	}

	for (uint x = 1; x < SAMPLES_PER_DIMENSION + 1; x++)
	{
		imageStore(glossySourceRadianceAtlas, ivec2(x, 0) + realTextureOffset, bottomDiffuseIrradianceEdge[SAMPLES_PER_PROBE_DIMENSION - x]);
		imageStore(glossySourceDepthAtlas,  ivec2(x, 0) + realTextureOffset, bottomDepthEdge[SAMPLES_PER_PROBE_DIMENSION - x]);

		imageStore(glossySourceRadianceAtlas, ivec2(x, SAMPLES_PER_DIMENSION + 1) + realTextureOffset, topDiffuseIrradianceEdge[SAMPLES_PER_PROBE_DIMENSION - x]);
		imageStore(glossySourceDepthAtlas,  ivec2(x, SAMPLES_PER_DIMENSION + 1) + realTextureOffset, topDepthEdge[SAMPLES_PER_PROBE_DIMENSION - x]);
	} 

	for (uint y = 1; y < SAMPLES_PER_DIMENSION + 1; y++)
	{
		imageStore(glossySourceRadianceAtlas, ivec2(0, y) + realTextureOffset, leftDiffuseIrradianceEdge[SAMPLES_PER_PROBE_DIMENSION - y]);
		imageStore(glossySourceDepthAtlas,  ivec2(0, y) + realTextureOffset, leftDepthEdge[SAMPLES_PER_PROBE_DIMENSION - y]);

		imageStore(glossySourceRadianceAtlas, ivec2(SAMPLES_PER_DIMENSION + 1, y) + realTextureOffset, rightDiffuseIrradianceEdge[SAMPLES_PER_PROBE_DIMENSION - y]);
		imageStore(glossySourceDepthAtlas,  ivec2(SAMPLES_PER_DIMENSION + 1, y) + realTextureOffset, rightDepthEdge[SAMPLES_PER_PROBE_DIMENSION - y]);
	} 

	//Corners Diffuse Irradiance
	imageStore(glossySourceRadianceAtlas, ivec2(0, 0) + realTextureOffset, topDiffuseIrradianceEdge[SAMPLES_PER_DIMENSION - 1]);
	imageStore(glossySourceRadianceAtlas, ivec2(0, SAMPLES_PER_DIMENSION + 1) + realTextureOffset, bottomDiffuseIrradianceEdge[SAMPLES_PER_DIMENSION - 1]);
	imageStore(glossySourceRadianceAtlas, ivec2(SAMPLES_PER_DIMENSION + 1, 0) + realTextureOffset, topDiffuseIrradianceEdge[0]);
	imageStore(glossySourceRadianceAtlas, ivec2(SAMPLES_PER_DIMENSION + 1, SAMPLES_PER_DIMENSION + 1) + realTextureOffset, bottomDiffuseIrradianceEdge[0]);

	//Corners Depth
	imageStore(glossySourceDepthAtlas, ivec2(0, 0) + realTextureOffset, topDepthEdge[SAMPLES_PER_DIMENSION - 1]);
	imageStore(glossySourceDepthAtlas, ivec2(0, SAMPLES_PER_DIMENSION + 1) + realTextureOffset, bottomDepthEdge[SAMPLES_PER_DIMENSION - 1]);
	imageStore(glossySourceDepthAtlas, ivec2(SAMPLES_PER_DIMENSION + 1, 0) + realTextureOffset, topDepthEdge[0]);
	imageStore(glossySourceDepthAtlas, ivec2(SAMPLES_PER_DIMENSION + 1, SAMPLES_PER_DIMENSION + 1) + realTextureOffset, bottomDepthEdge[0]);
}