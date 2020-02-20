#version 460
#extension GL_NV_ray_tracing : require

#define M_PI 3.1415926535897932384626433832795f

layout (constant_id = 0) const uint WORLD_SIZE_X = 10;
layout (constant_id = 1) const uint WORLD_SIZE_Y = 10;
layout (constant_id = 2) const uint WORLD_SIZE_Z = 10;
layout (constant_id = 3) const uint SAMPLES_PER_PROBE = 64;
layout (constant_id = 4) const uint NUM_PROBES_PER_DIMENSION = 3;

layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;
layout(binding = 9, set = 0) buffer LightProbeValues { vec4 colors[]; } lightProbeValues;

struct RayPayload 
{
	vec3 color;
	uint recursion;
};

layout(location = 0) rayPayloadNV RayPayload rayPayload;

vec3 createSampleDirection(uint n, float gr)
{
	// float index = float(n) + 0.5f;
	// float phi = acos(1.0f  - 2.0f * index / SAMPLES_PER_PROBE);
	// float theta = M_PI * index * (1.0f + sqrt5);
	// return vec3(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));

	// float i = float(n + 1);
	// float lat = asin(-1.0f + 2.0f * float(i) / (SAMPLES_PER_PROBE + 1));
	// float lon = ga * i;

	// return vec3(cos(lon) * cos(lat), sin(lon) * cos(lat), sin(lat));

	float z = 1.0f - 2.0f * n / SAMPLES_PER_PROBE;
	float theta = acos(z);
	float phi = 2.0f * n * M_PI * gr;

	return vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), z);
}

vec3 hsv2rgb(vec3 c) 
{
	vec4 K = vec4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0f - K.www);
	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0f, 1.0f), c.y);
}


void main() 
{
	vec3 worldSize = vec3(WORLD_SIZE_X, WORLD_SIZE_Y, WORLD_SIZE_Z);
	vec3 lightProbeCenter = (gl_LaunchIDNV / vec3(NUM_PROBES_PER_DIMENSION - 1)) * worldSize - worldSize / 2.0f;
	
	float sqrt5 = sqrt(5.0f);

	uint baseLightProbeIndex = (gl_LaunchIDNV.x + NUM_PROBES_PER_DIMENSION * (gl_LaunchIDNV.y + NUM_PROBES_PER_DIMENSION * gl_LaunchIDNV.z)) * SAMPLES_PER_PROBE;

	float gr=(sqrt(5.0) + 1.0) / 2.0;  // golden ratio = 1.6180339887498948482
    float ga=(2.0 - gr) * (2.0*M_PI);  // golden angle = 2.39996322972865332

	for (uint n = 0; n < SAMPLES_PER_PROBE; n++)
	{
		vec3 rayDirection = createSampleDirection(n, gr);

		uint rayFlags = gl_RayFlagsOpaqueNV;
		uint cullMask = 0x80;
		float tmin = 0.001f;
		float tmax = 10000.0f;

		rayPayload.color = vec3(0.0f);
		rayPayload.recursion = 0;
		traceNV(topLevelAS, rayFlags, cullMask, 0, 0, 0, lightProbeCenter, tmin, rayDirection, tmax, 0);
		lightProbeValues.colors[baseLightProbeIndex + n] = vec4(rayPayload.color, 1.0f);
		//lightProbeValues.colors[baseLightProbeIndex + n] = vec4(hsv2rgb(vec3(float(n) / float(SAMPLES_PER_PROBE), 1.0f, 1.0f)), 1.0f);
	}
}