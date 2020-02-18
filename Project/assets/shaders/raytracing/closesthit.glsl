#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

#define M_PI 3.1415926535897932384626433832795f

struct RayPayload
{
	vec3 color;
	uint recursion;
};

struct ShadowRayPayload
{
	float lightIntensity;
};

struct Vertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
	vec4 TexCoord;
};

layout(location = 0) rayPayloadInNV RayPayload rayPayload;
layout(location = 1) rayPayloadNV ShadowRayPayload shadowRayPayload;

hitAttributeNV vec3 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;
layout(binding = 2, set = 0) uniform CameraProperties
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 lightPos;
} cam;
layout(binding = 3, set = 0) buffer Vertices { Vertex v[]; } vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices;
layout(binding = 5, set = 0) buffer MeshIndices { uint mi[]; } meshIndices;
layout(binding = 6, set = 0) uniform sampler2D albedoMaps[16];
layout(binding = 7, set = 0) uniform sampler2D normalMaps[16];
layout(binding = 8, set = 0) uniform sampler2D roughnessMaps[16];

// Max. number of recursion is passed via a specialization constant
layout (constant_id = 0) const int MAX_RECURSION = 0;

vec3 myRefract(vec3 I, vec3 N, float ior)
{
    float cosi = clamp(-1.0f, 1.0f, dot(I, N));
    float etai = 1;
	float etat = ior;
    vec3 n = N;
    if (cosi < 0)
	{
		cosi = -cosi;
	}
	else
	{
		float temp = etai;
		etai = etat;
		etat = temp;
		n = -N;
	}
    float eta = etai / etat;
    float k = 1.0f - eta * eta * (1.0f - cosi * cosi);
    return k < 0.0f ? vec3(0.0f) : eta * I + (eta * cosi - sqrt(k)) * n;
}

float fresnel(vec3 I, vec3 N, float ior)
{
    float cosi = clamp(-1.0f, 1.0f, dot(I, N));
    float etai = 1;
	float etat = ior;

    if (cosi > 0)
	{
		float temp = etai;
		etai = etat;
		etat = temp;
	}

    // Compute sini using Snell's law
    float sint = etai / etat * sqrt(max(0.0f, 1.0f - cosi * cosi));
    // Total internal reflection
    if (sint >= 1.0f)
	{
        return 1.0f;
    }
    else
	{
        float cost = sqrt(max(0.0f, 1.0f - sint * sint));
        cosi = abs(cosi);
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        return (Rs * Rs + Rp * Rp) / 2;
    }
    // As a consequence of the conservation of energy, transmittance is given by:
    // kt = 1 - kr;
}

float rand(vec3 v)
{
	return fract(tan(distance(v.xy * 1.61803398874989484820459f, v.xy) * v.z) * v.x);
}

float hash(float n)
{
    return fract(sin(n) * 43758.5453f);
}

float noise(vec3 x)
{
    // The noise function returns a value in the range -1.0f -> 1.0f

    vec3 p = floor(x);
    vec3 f = fract(x);

    f       = f * f * (3.0f - 2.0f * f);
    float n = p.x + p.y * 57.0f + 113.0 * p.z;

    return mix(mix(mix(hash(n + 0.0f), hash(n + 1.0f), f.x),
                   mix(hash(n + 57.0f), hash(n + 58.0f), f.x), f.y),
               mix(mix(hash(n + 113.0f), hash(n + 114.0f), f.x),
                   mix(hash(n + 170.0f), hash(n + 171.0f), f.x), f.y), f.z) * 0.5f + 0.5f;
}

vec3 findNT(vec3 normal)
{
	if (abs(normal.x) > abs(normal.y))
		return normalize(vec3(normal.z, 0.0f, -normal.x));
	
	return normalize(vec3(0.0f, -normal.z, normal.y));
}

vec3 createSampleDirection(float uniformRandom1, float uniformRandom2)
{
	float sinTheta = sqrt(1.0f - uniformRandom1 * uniformRandom1); 
    float phi = 2.0f * M_PI * uniformRandom2; 
    float x = sinTheta * cos(phi); 
    float z = sinTheta * sin(phi); 
    return vec3(x, uniformRandom1, z); 
}

void main()
{
	uint recursionIndex = rayPayload.recursion;

	uint meshVertexOffset = meshIndices.mi[3 * gl_InstanceCustomIndexNV];
	uint meshIndexOffset = 	meshIndices.mi[3 * gl_InstanceCustomIndexNV + 1];
	uint textureIndex = 	meshIndices.mi[3 * gl_InstanceCustomIndexNV + 2];
	ivec3 index = ivec3(indices.i[meshIndexOffset + 3 * gl_PrimitiveID], indices.i[meshIndexOffset + 3 * gl_PrimitiveID + 1], indices.i[meshIndexOffset + 3 * gl_PrimitiveID + 2]);

	Vertex v0 = vertices.v[meshVertexOffset + index.x];
	Vertex v1 = vertices.v[meshVertexOffset + index.y];
	Vertex v2 = vertices.v[meshVertexOffset + index.z];

	// Interpolate normal
	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	vec2 texCoords = (v0.TexCoord.xy * barycentricCoords.x + v1.TexCoord.xy * barycentricCoords.y + v2.TexCoord.xy * barycentricCoords.z);
	mat4 transform;
	transform[0] = vec4(gl_ObjectToWorldNV[0], 0.0f);
	transform[1] = vec4(gl_ObjectToWorldNV[1], 0.0f);
	transform[2] = vec4(gl_ObjectToWorldNV[2], 0.0f);
	transform[3] = vec4(gl_ObjectToWorldNV[3], 1.0f);

	vec3 T = normalize(v0.Tangent.xyz * barycentricCoords.x + v1.Tangent.xyz * barycentricCoords.y + v2.Tangent.xyz * barycentricCoords.z);
	vec3 N  = normalize(v0.Normal.xyz * barycentricCoords.x + v1.Normal.xyz * barycentricCoords.y + v2.Normal.xyz * barycentricCoords.z);

	T = normalize(vec3(transform * vec4(T, 0.0)));
	N = normalize(vec3(transform * vec4(N, 0.0)));
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);

	vec3 normal = texture(normalMaps[textureIndex], texCoords).xyz;
	normal = normalize(normal * 2.0f - 1.0f);
	normal = TBN * normal;

	vec4 albedoColor = texture(albedoMaps[textureIndex], texCoords);
	float roughness = texture(roughnessMaps[textureIndex], texCoords).r;

	if (recursionIndex < MAX_RECURSION)
	{
		vec3 hitPos = gl_WorldRayOriginNV + normalize(gl_WorldRayDirectionNV) * gl_HitTNV;
		uint rayFlags = gl_RayFlagsOpaqueNV;
		uint cullMask = 0xff;
		float tmin = 0.001;
		float tmax = 10000.0;

		vec3 Nt = findNT(normal);
		vec3 Nb = cross(normal, Nt);

		vec3 directDiffuse = vec3(0.0f);
		vec3 indirectDiffuse = vec3(0.0f);

		vec3 lightPos = vec3(0.0f, 5.0f, 0.0f);
		vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

		vec3 shadowOrigin = hitPos + normal * 0.001f;
		vec3 lightDirection = normalize(lightPos - shadowOrigin);
		traceNV(topLevelAS, rayFlags, cullMask, 1, 0, 1, shadowOrigin, tmin, lightDirection, tmax, 1);

		directDiffuse = lightColor * shadowRayPayload.lightIntensity * max(0.0f, dot(normal, -lightDirection)); 

		uint NUM_SAMPLES = 64;
		for (uint n = 0; n < NUM_SAMPLES; n++)
		{
			vec3 seed = vec3(n, n * n, n * n * n);
			float uniformRandom1 = noise(hitPos + seed.xyz);
			float uniformRandom2 = noise(hitPos + seed.zyx);
			vec3 sdl = createSampleDirection(uniformRandom1, uniformRandom2); //Sample Direction Local
			vec3 sdw = vec3(
				sdl.x * Nb.x + sdl.y * normal.x + sdl.z * Nt.x,
				sdl.x * Nb.y + sdl.y * normal.y + sdl.z * Nt.y,
				sdl.x * Nb.z + sdl.y * normal.z + sdl.z * Nt.z);
			
			vec3 rayOrigin = hitPos + sdw * 0.001f;
			rayPayload.recursion = recursionIndex + 1;
			traceNV(topLevelAS, rayFlags, cullMask, 0, 0, 0, rayOrigin, tmin, sdw, tmax, 0);
			indirectDiffuse += uniformRandom1 * rayPayload.color;
		}

		indirectDiffuse = indirectDiffuse / float(NUM_SAMPLES); 
		rayPayload.color = (directDiffuse / M_PI + 2.0f * indirectDiffuse) * albedoColor.rgb; 
	}
	else
	{
		rayPayload.color = vec3(0.0f); 
	}
}
