#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

struct RayPayload 
{
	vec3 color;
	uint recursion;
	float occluderFactor;
	// float distance;
	// vec3 normal;
	// float reflector;
	// float refractor;
};

struct Vertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
	vec4 TexCoord;
};

layout(location = 0) rayPayloadInNV RayPayload rayPayload;

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

void main()
{
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

	vec3 tangent = normalize(v0.Tangent.xyz * barycentricCoords.x + v1.Tangent.xyz * barycentricCoords.y + v2.Tangent.xyz * barycentricCoords.z);
	vec3 normal  = normalize(v0.Normal.xyz * barycentricCoords.x + v1.Normal.xyz * barycentricCoords.y + v2.Normal.xyz * barycentricCoords.z);

	vec3 T = normalize(vec3(transform * vec4(tangent, 0.0)));
	vec3 N = normalize(vec3(transform * vec4(normal, 0.0)));
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);
	
	vec3 worldNormal = texture(normalMaps[textureIndex], texCoords).xyz;
	worldNormal = normalize(worldNormal * 2.0f - 1.0f); 
	worldNormal = TBN * worldNormal;
	
	vec4 albedoColor = texture(albedoMaps[textureIndex], texCoords);
	float refractiveness = (1.0f - albedoColor.a);
	float reflectiveness = (1.0f - texture(roughnessMaps[textureIndex], texCoords).r);

	// // Basic lighting
	// vec3 lightVector = normalize(cam.lightPos.xyz);
	// float dot_product = max(dot(lightVector, worldNormal), 0.6);
	// rayPayload.color = albedoColor.rgb;
	// rayPayload.distance = gl_RayTmaxNV;
	// rayPayload.normal = worldNormal;
	// rayPayload.refractor = (1.0f - albedoColor.a);
	// rayPayload.reflector = reflectiveness; //* rayPayload.refractor;

	vec3 hitPos = gl_WorldRayOriginNV + normalize(gl_WorldRayDirectionNV) * gl_HitTNV;
	uint rayFlags = gl_RayFlagsOpaqueNV;
	uint cullMask = 0xff;
	float tmin = 0.001;
	float tmax = 10000.0;
	
	uint recursionNumber = rayPayload.recursion;

	if (recursionNumber < MAX_RECURSION)
	{
		float shadow = 0.0f;
		vec3 shadowOrigin = hitPos + worldNormal * 0.001f;
		vec3 shadowDirection = normalize(vec3(0.0f, 5.0f, 0.0f) - shadowOrigin);
		rayPayload.recursion = recursionNumber + 1;
		traceNV(topLevelAS, rayFlags, cullMask, 0, 0, 0, shadowOrigin, tmin, shadowDirection, tmax, 0);
		shadow = (1.0f - rayPayload.occluderFactor);
	
		if (refractiveness > 0.1f)
		{
			float refracionIndex = 1.16f;
			float NdotD = dot(worldNormal, gl_WorldRayDirectionNV);

			vec3 refrNormal = worldNormal;
			float refrEta;

			vec3 refractionOrigin;
			vec3 reflectionOrigin;

			if(NdotD >= 0.0f) 
			{
				refrNormal = -worldNormal;
				refrEta = 1.0f / refracionIndex;
				refractionOrigin = hitPos + worldNormal * 0.001f;
				reflectionOrigin = hitPos - worldNormal * 0.001f;
			} 
			else 
			{
				refrNormal = worldNormal;
				refrEta = refracionIndex;
				refractionOrigin = hitPos - worldNormal * 0.001f;
				reflectionOrigin = hitPos + worldNormal * 0.001f;
			}

			vec3 refractionColor = vec3(0.0f);
			float kr = fresnel(gl_WorldRayDirectionNV, worldNormal, refracionIndex);
			if (kr < 1)
			{
				vec3 refractionDirection = myRefract(gl_WorldRayDirectionNV, refrNormal, refrEta);
				rayPayload.recursion = recursionNumber + 1;
				traceNV(topLevelAS, rayFlags, cullMask, 0, 0, 0, refractionOrigin, tmin, refractionDirection, tmax, 0);
				refractionColor = (1.0f - kr) * rayPayload.color;
			}

			vec3 reflectionDirection = reflect(gl_WorldRayDirectionNV, worldNormal);
			rayPayload.recursion = recursionNumber + 1;
			traceNV(topLevelAS, rayFlags, cullMask, 0, 0, 0, reflectionOrigin, tmin, reflectionDirection, tmax, 0);
			vec3 reflectionColor = kr * rayPayload.color;

			rayPayload.color = shadow * (refractiveness * (refractionColor + reflectionColor) + (1.0f - refractiveness) * albedoColor.rgb);
		}
		else if (reflectiveness > 0.1f)
		{
			vec3 origin = hitPos + worldNormal * 0.001f;
			vec3 reflectionDirection = reflect(gl_WorldRayDirectionNV, worldNormal);
			rayPayload.recursion = recursionNumber + 1;

			traceNV(topLevelAS, rayFlags, cullMask, 0, 0, 0, origin, tmin, reflectionDirection, tmax, 0);
			rayPayload.color = shadow * (reflectiveness * rayPayload.color + (1.0f - reflectiveness) * albedoColor.rgb);
		}

		//float albedoColorLoss = min(reflectiveness + refractiveness, 1.0f);
		//rayPayload.color = min((1.0f - albedoColorLoss) * albedoColor.rgb + refractionColor + reflectionColor, 1.0f);
	}
	else
	{
		rayPayload.color = albedoColor.rgb;
	}

	float occluderBaseFactor = 0.7f;
	rayPayload.occluderFactor = (1.0f - refractiveness) * occluderBaseFactor;//refractiveness > 0.1f ? occluderBaseFactor * refractiveness : occluderBaseFactor * reflectiveness;

	
	//rayPayload.color = albedoColor.rgb;
}
