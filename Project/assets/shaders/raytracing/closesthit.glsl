#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

struct RayPayload
{
	vec3 color;
	uint recursion;
};

struct ShadowRayPayload
{
	float occluderFactor;
};

struct Vertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
	vec4 TexCoord;
};

struct MaterialParameters
{
	vec4 Albedo;
	float Metallic;
	float Roughness;
	float AO;
	float Padding;
};

struct PointLight
{
	vec4 Color;
	vec4 Position;
};

layout(location = 0) rayPayloadInNV RayPayload rayPayload;
layout(location = 1) rayPayloadNV ShadowRayPayload shadowRayPayload;

hitAttributeNV vec3 attribs;

// Max. number of recursion is passed via a specialization constant
layout (constant_id = 0) const int MAX_RECURSION = 0;
layout (constant_id = 1) const int MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES = 16;
layout (constant_id = 2) const int MAX_POINT_LIGHTS = 4;

layout(binding = 2, set = 0) uniform accelerationStructureNV topLevelAS;
layout(binding = 6, set = 0) buffer Vertices { Vertex v[]; } vertices;
layout(binding = 7, set = 0) buffer Indices { uint i[]; } indices;
layout(binding = 8, set = 0) buffer MeshIndices { uint mi[]; } meshIndices;
layout(binding = 9 , set = 0) uniform sampler2D albedoMaps[MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES];
layout(binding = 10, set = 0) uniform sampler2D normalMaps[MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES];
layout(binding = 11, set = 0) uniform sampler2D aoMaps[MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES];
layout(binding = 12, set = 0) uniform sampler2D metallicMaps[MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES];
layout(binding = 13, set = 0) uniform sampler2D roughnessMaps[MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES];
layout(binding = 14, set = 0) buffer CombinedMaterialParameters 
{ 
	MaterialParameters mp[]; 
} u_MaterialParameters;
layout (binding = 16) uniform LightBuffer
{
	PointLight lights[MAX_POINT_LIGHTS];
} u_Lights;

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

float rand(float n)
{
	return (fract(sin(n) * 43758.5453123f) - 0.5f) * 2.0f;
}

vec3 findBasisVector(vec3 normal)
{
	if (abs(normal.y) > 0.0f && abs(normal.z) > 0.0f)
		return vec3(1.0f, 0.0f, 0.0f);

	if (abs(normal.x) > 0.0f && abs(normal.z) > 0.0f)
		return vec3(0.0f, 1.0f, 0.0f);

	if (abs(normal.x) > 0.0f && abs(normal.y) > 0.0f)
		return vec3(0.0f, 0.0f, 1.0f);
}

void calculateTriangleData(out uint materialIndex, out vec2 texCoords, out vec3 normal)
{
	materialIndex = 	meshIndices.mi[3 * gl_InstanceCustomIndexNV + 2];

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

	normal = texture(normalMaps[materialIndex], texCoords).xyz;
	normal = normalize(normal * 2.0f - 1.0f);
	normal = TBN * normal;
}

void main()
{
	uint materialIndex = 0;
	vec2 texCoords = vec2(0.0f);
	vec3 normal = vec3(0.0f);
	calculateTriangleData(materialIndex, texCoords, normal);

	vec4 albedo = texture(albedoMaps[materialIndex], texCoords);
	vec4 ao = texture(aoMaps[materialIndex], texCoords);
	vec4 metallic = texture(metallicMaps[materialIndex], texCoords);
	vec4 roughness = texture(roughnessMaps[materialIndex], texCoords);
	MaterialParameters mp = u_MaterialParameters.mp[materialIndex];

	float refractiveness = (1.0f - albedo.a);
	float reflectiveness = (1.0f - mp.Roughness * roughness.r);

	vec3 hitPos = gl_WorldRayOriginNV + normalize(gl_WorldRayDirectionNV) * gl_HitTNV;
	uint rayFlags = gl_RayFlagsOpaqueNV;
	uint cullMask = 0xff;
	float tmin = 0.001;
	float tmax = 10000.0;

	uint recursionNumber = rayPayload.recursion;

	uint sbtStride = 0;

	if (recursionNumber < MAX_RECURSION)
	{
		if (refractiveness > 0.1f)
		{
			float refracionIndex = 1.16f;
			float NdotD = dot(normal, gl_WorldRayDirectionNV);

			vec3 refrNormal = normal;
			float refrEta;

			vec3 refractionOrigin;
			vec3 reflectionOrigin;

			if(NdotD >= 0.0f)
			{
				refrNormal = -normal;
				refrEta = 1.0f / refracionIndex;
				refractionOrigin = hitPos + normal * 0.001f;
				reflectionOrigin = hitPos - normal * 0.001f;
			}
			else
			{
				refrNormal = normal;
				refrEta = refracionIndex;
				refractionOrigin = hitPos - normal * 0.001f;
				reflectionOrigin = hitPos + normal * 0.001f;
			}

			vec3 refractionColor = vec3(0.0f);
			float kr = fresnel(gl_WorldRayDirectionNV, normal, refracionIndex);
			if (kr < 1)
			{
				vec3 refractionDirection = myRefract(gl_WorldRayDirectionNV, refrNormal, refrEta);
				rayPayload.recursion = recursionNumber + 1;
				traceNV(topLevelAS, rayFlags, cullMask, 0, sbtStride, 0, refractionOrigin, tmin, refractionDirection, tmax, 0);
				refractionColor = (1.0f - kr) * rayPayload.color;
			}

			vec3 reflectionDirection = reflect(gl_WorldRayDirectionNV, normal);
			rayPayload.recursion = recursionNumber + 1;
			traceNV(topLevelAS, rayFlags, cullMask, 0, sbtStride, 0, reflectionOrigin, tmin, reflectionDirection, tmax, 0);
			vec3 reflectionColor = kr * rayPayload.color;

			rayPayload.color = refractiveness * (refractionColor + reflectionColor) + (1.0f - refractiveness) * mp.Albedo.rgb * albedo.rgb;
		}
		else if (reflectiveness > 0.1f)
		{
			vec3 lightPos = vec3(0.0f, 5.0f, 0.0f);
			// float lightRadius = 0.2f;

			// vec3 shadowOrigin = hitPos + normal * 0.001f;
			// vec3 lightToHit = normalize(shadowOrigin - lightPos);
			// vec3 u = findBasisVector(lightToHit);
			// vec3 v = normalize(cross(u, lightToHit));
			// vec3 offsetU = vec3(0.0f);
			// vec3 offsetV = vec3(0.0f);

			// float shadowSum = 0.0f;
			// uint numShadowSamples = 12;

			// float seed = dot(shadowOrigin, shadowOrigin);

			// for (uint i = 0; i < numShadowSamples; i++)
			// {
			// 	vec3 lightSamplePoint = lightPos + offsetU + offsetV;
			// 	vec3 shadowDirection = normalize(lightSamplePoint - shadowOrigin);

			// 	traceNV(topLevelAS, rayFlags, cullMask, 1, sbtStride, 1, shadowOrigin, tmin, shadowDirection, tmax, 1);
			// 	shadowSum += shadowRayPayload.occluderFactor;

			// 	offsetU = u * rand(seed + i) * lightRadius;
			// 	offsetV = v * rand(seed + i * i) * lightRadius;
			// }

			// float shadowDarkness = 0.8f;
			// float shadow = (1.0f - (shadowSum / float(numShadowSamples)) * shadowDarkness);

			vec3 directLightOrigin = hitPos + normal * 0.001f;
			vec3 finalLightColor = vec3(0.0f);

			for (int i = 0; i < MAX_POINT_LIGHTS; i++)
			{
				vec3 lightPosition 	= u_Lights.lights[i].Position.xyz;
				vec3 lightColor 	= u_Lights.lights[i].Color.rgb;

				vec3 lightDirection = (lightPosition - directLightOrigin);
				float distance 		= length(lightDirection);
				float attenuation 	= 1.0f / (distance * distance);

				lightDirection 	= normalize(lightDirection);

				traceNV(topLevelAS, rayFlags, cullMask, 1, sbtStride, 1, directLightOrigin, tmin, lightDirection, tmax, 1);

				float tempFactor = 0.8f;
				finalLightColor += attenuation * lightColor * (1.0f - shadowRayPayload.occluderFactor * tempFactor);
			}

			vec3 origin = hitPos + normal * 0.001f;
			vec3 reflectionDirection = reflect(gl_WorldRayDirectionNV, normal);
			rayPayload.recursion = recursionNumber + 1;

			traceNV(topLevelAS, rayFlags, cullMask, 0, sbtStride, 0, origin, tmin, reflectionDirection, tmax, 0);
			rayPayload.color = finalLightColor * (reflectiveness * rayPayload.color + (1.0f - reflectiveness) * mp.Albedo.rgb * albedo.rgb);
		}
	}
	else
	{
		rayPayload.color = mp.Albedo.rgb * albedo.rgb;
	}
}
