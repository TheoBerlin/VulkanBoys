#version 450

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(set = 0, binding = 0, rgba16f) writeonly uniform image2D u_IntegrationLUT;

const uint  SAMPLE_COUNT    = 1024U;
const float PI              = 3.14159265359f;

/*
    http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
*/
float RadicalInverseVdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

/*
    i - the current sample
    n - total number of samples 
*/
vec2 Hammersley(uint i, uint n)
{
    return vec2(float(i) / float(n), RadicalInverseVdC(i));
}

/*
    Retrive sample vector roughly oriented around the microsurface halfVector
*/
vec3 ImportanceSample(vec2 Xi, vec3 normal, float roughness)
{
    float alpha     = roughness * roughness;
    float phi       = 2.0f * PI * Xi.x;
    float cosTheta  = sqrt((1.0f - Xi.y) / (1.0f + (alpha * alpha - 1.0f) * Xi.y));
    float sinTheta  = sqrt(1.0f - cosTheta * cosTheta);

    vec3 halfVector = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up         = (abs(normal.z) < 0.999) ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
    vec3 tangent    = normalize(cross(up, normal));
    vec3 bitangent  = normalize(cross(normal, tangent));

    vec3 sampleVector = (tangent * halfVector.x) + (bitangent * halfVector.y) + (normal * halfVector.z);
    return normalize(sampleVector);
}

/*
	Schlick and GGX Geometry-Function
*/
float GeometryGGX(float dotNV, float roughness)
{
	float k_IBL = (roughness * roughness) / 2.0f;	
	return dotNV / (dotNV * (1.0f - k_IBL) + k_IBL);
}

/*
	Smith Geometry-Function
*/
float Geometry(vec3 normal, vec3 viewDir, vec3 lightDirection, float roughness)
{
	float dotNV = max(dot(normal, viewDir), 0.0f);
	float dotNL = max(dot(normal, lightDirection), 0.0f);
	
	return GeometryGGX(dotNV, roughness) * GeometryGGX(dotNL, roughness);
}

vec2 IntegrateBRDF(float dotNV, float roughness)
{
    vec3 view = vec3(sqrt(1.0f - (dotNV * dotNV)), 0.0f, dotNV);

    float a = 0.0f;
    float b = 0.0f;

    vec3 normal = vec3(0.0f, 0.0f, 1.0f);
    for (uint i = 0; i < SAMPLE_COUNT; i++)
    {
        vec2 Xi         = Hammersley(i, SAMPLE_COUNT);
        vec3 halfVector = ImportanceSample(Xi, normal, roughness);
        vec3 light      = normalize(2.0f * dot(view, halfVector) * halfVector - view);

        float dotNL = max(light.z, 0.0f);
        float dotNH = max(halfVector.z, 0.0f);
        float dotVH = max(dot(view, halfVector), 0.0f);

        if (dotNL > 0.0f)
        {
            float g     = Geometry(normal, view, light, roughness);
            float g_vis = (g * dotVH) / (dotNH * dotNV);
            float fc    = pow(1.0f - dotVH, 5.0f);

            a += (1.0f - fc) * g_vis;
            b += fc * g_vis;
        }
    }

    a = a / float(SAMPLE_COUNT);
    b = b / float(SAMPLE_COUNT);
    return vec2(a, b);
}

void main()
{
    ivec2 texelCoord    = ivec2(gl_WorkGroupID.xy);
    vec2 imageSize      = vec2(gl_NumWorkGroups.xy);
    vec2 texCoord       = (vec2(texelCoord) + vec2(0.5f))/ imageSize;

    vec2 integratedBDRF = IntegrateBRDF(texCoord.x, 1.0f - texCoord.y);
    imageStore(u_IntegrationLUT, texelCoord, vec4(integratedBDRF, 0.0f, 1.0f));
}