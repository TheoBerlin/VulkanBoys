#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in     vec3 in_Position;
layout(location = 0) out    vec4 out_Color;

layout(binding = 1) uniform samplerCube u_EnvMap;

layout (push_constant) uniform Constants
{
	mat4    View;
	float   Roughness;
} constants;

const uint  SAMPLE_COUNT    = 4096;
const float PI              = 3.14159265359f;

/*
	GGX Distribution function
*/
float Distribution(vec3 normal, vec3 halfVector, float roughness)
{
	float alpha         = roughness*roughness;
	float alphaSqrd 	= alpha *alpha;
	
	float dotNH = max(dot(normal, halfVector), 0.0f);
	
	float denom = ((dotNH * dotNH) * (alphaSqrd - 1.0f)) + 1.0f;
	return alphaSqrd / (PI * denom * denom);
}

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
vec3 ImportanceSample(vec2 xi, vec3 normal, float roughness)
{
    float alpha     = roughness * roughness;

    float phi       = 2.0f * PI * xi.x;
    float cosTheta  = sqrt((1.0f - xi.y) / (1.0f + ((alpha * alpha) - 1.0f) * xi.y));
    float sinTheta  = sqrt(1.0f - cosTheta * cosTheta);

    vec3 halfVector = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up         = (abs(normal.z) < 0.999) ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
    vec3 tangent    = normalize(cross(up, normal));
    vec3 bitangent  = cross(normal, tangent);

    vec3 sampleVector = (tangent * halfVector.x) + (bitangent * halfVector.y) + (normal * halfVector.z);
    return normalize(sampleVector);
}

void main()
{
    vec3 normal = normalize(in_Position);
    vec3 view   = normal;

    float   totalWeight         = 0.0f;
    vec3    preFilteredColor    = vec3(0.0f);
    for (uint i = 0U; i < SAMPLE_COUNT; i++)
    {
        vec2 xi         = Hammersley(i, SAMPLE_COUNT);
        vec3 halfVector = ImportanceSample(xi, normal, constants.Roughness);
        vec3 light      = (2.0f * dot(view, halfVector) * halfVector) - view;

        float dotNL = clamp(dot(normal, light), 0.0f, 1.0f);
        if (dotNL > 0.0f)
        {
            float d     = Distribution(normal, halfVector, constants.Roughness);
            float dotNH = clamp(dot(normal, halfVector), 0.0f, 1.0f);
            float dotHV = clamp(dot(halfVector, view), 0.0f, 1.0f);
            float pdf   = d * dotNH / (4.0f * dotHV) + 0.0001f; 

            const float RESOLUTION = 2048.0f;
            float saTexel       = 4.0f * PI / (6.0 * RESOLUTION * RESOLUTION);
            float saSample      = 1.0f / (float(SAMPLE_COUNT) * pdf + 0.0001f);

            float mipLevel = (constants.Roughness == 0.0f) ? 0.0f : 0.5f * log2(saSample / saTexel); 

            preFilteredColor += textureLod(u_EnvMap, light, mipLevel).rgb * dotNL;
            totalWeight      += dotNL;
        }
    }

    preFilteredColor    = preFilteredColor / totalWeight;
    out_Color           = vec4(preFilteredColor, 1.0f);
}