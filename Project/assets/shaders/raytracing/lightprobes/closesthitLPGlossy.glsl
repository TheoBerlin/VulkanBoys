#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

#define M_PI 3.1415926535897932384626433832795f

struct RayPayload
{
	vec3 color;
	float distance;
	uint recursion;
};

// struct ShadowRayPayload
// {
// 	float lightIntensity;
// };

struct Vertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
	vec4 TexCoord;
};

layout (constant_id = 6) const uint MAX_RECURSIONS = 3;

layout(location = 0) rayPayloadInNV RayPayload rayPayload;
//layout(location = 1) rayPayloadNV ShadowRayPayload shadowRayPayload;

hitAttributeNV vec3 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;

layout(binding = 3, set = 0) buffer Vertices { Vertex v[]; } vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices;
layout(binding = 5, set = 0) buffer MeshIndices { uint mi[]; } meshIndices;
layout(binding = 6, set = 0) uniform sampler2D albedoMaps[16];
layout(binding = 7, set = 0) uniform sampler2D normalMaps[16];
layout(binding = 8, set = 0) uniform sampler2D roughnessMaps[16];

void calculateTriangleData(out uint textureIndex, out vec2 texCoords, out vec3 normal)
{
	textureIndex = 	meshIndices.mi[3 * gl_InstanceCustomIndexNV + 2];

	uint meshVertexOffset = meshIndices.mi[3 * gl_InstanceCustomIndexNV];
	uint meshIndexOffset = 	meshIndices.mi[3 * gl_InstanceCustomIndexNV + 1];
	ivec3 index = ivec3(indices.i[meshIndexOffset + 3 * gl_PrimitiveID], indices.i[meshIndexOffset + 3 * gl_PrimitiveID + 1], indices.i[meshIndexOffset + 3 * gl_PrimitiveID + 2]);

	Vertex v0 = vertices.v[meshVertexOffset + index.x];
	Vertex v1 = vertices.v[meshVertexOffset + index.y];
	Vertex v2 = vertices.v[meshVertexOffset + index.z];

	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

	texCoords = (v0.TexCoord.xy * barycentricCoords.x + v1.TexCoord.xy * barycentricCoords.y + v2.TexCoord.xy * barycentricCoords.z);

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

	normal = texture(normalMaps[textureIndex], texCoords).xyz;
	normal = normalize(normal * 2.0f - 1.0f);
	normal = TBN * normal;
}

void main()
{
	uint textureIndex = 0;
	vec2 texCoords = vec2(0.0f);
	vec3 normal = vec3(0.0f);
	calculateTriangleData(textureIndex, texCoords, normal);

	uint recursionIndex = rayPayload.recursion;

	vec4 albedoColor = texture(albedoMaps[textureIndex], texCoords);
	float roughness = texture(roughnessMaps[textureIndex], texCoords).r;

	if (recursionIndex < MAX_RECURSIONS && roughness < 0.985f)
	{
		vec3 hitPos = gl_WorldRayOriginNV + normalize(gl_WorldRayDirectionNV) * gl_HitTNV;
		uint rayFlags = gl_RayFlagsOpaqueNV;
		uint cullMask = 0x80;
		float tmin = 0.001f;
		float tmax = 10000.0f;

		// vec3 shadowOrigin = hitPos + worldNormal * 0.001f;
		// vec3 shadowDirection = normalize(lightPos - shadowOrigin);
		// traceNV(topLevelAS, rayFlags, cullMask, 1, sbtStride, 1, shadowOrigin, tmin, shadowDirection, tmax, 1);

		// float shadowDarkness = 0.8f;
		// float shadow = 1.0f - shadowRayPayload.occluderFactor * shadowDarkness;

		vec3 origin = hitPos + normal * 0.001f;
		vec3 reflectionDirection = reflect(gl_WorldRayDirectionNV, normal);
		rayPayload.recursion = recursionIndex + 1;

		traceNV(topLevelAS, rayFlags, cullMask, 0, 0, 0, origin, tmin, reflectionDirection, tmax, 0);
		rayPayload.color = (1.0f - roughness) * rayPayload.color + roughness * albedoColor.rgb;
		//rayPayload.color = shadow * (reflectiveness * rayPayload.color + (1.0f - reflectiveness) * albedoColor.rgb);
	}
	else
	{
		rayPayload.color = albedoColor.rgb; 
	}

	rayPayload.distance = gl_HitTNV;
}
