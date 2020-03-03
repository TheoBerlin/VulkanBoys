#version 460

#include "helpers.glsl"

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

layout (constant_id = 1) const int NUM_PROBES_PER_DIMENSION = 2;
layout (constant_id = 2) const int SAMPLES_PER_PROBE_DIMENSION_GLOSSY_SOURCE = 16;
layout (constant_id = 3) const int SAMPLES_PER_PROBE_GLOSSY_SOURCE = 256;
layout (constant_id = 4) const int SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI = 8;
layout (constant_id = 5) const int SAMPLES_PER_PROBE_COLLAPSED_GI = 64;

layout (binding = 11, set = 0) uniform sampler2D glossySourceRadianceAtlas;
layout (binding = 12, set = 0) uniform sampler2D glossySourceDepthAtlas;
layout (binding = 13, set = 0, r11f_g11f_b10f) uniform image2D collapsedGIIrradianceAtlas;
layout (binding = 14, set = 0, rg16f) uniform image2D collapsedGIDepthAtlas;

void main()
{
    const uint TOTAL_NUMBER_OF_PROBES = NUM_PROBES_PER_DIMENSION * NUM_PROBES_PER_DIMENSION * NUM_PROBES_PER_DIMENSION;

    if (gl_GlobalInvocationID.x >= SAMPLES_PER_PROBE_COLLAPSED_GI * TOTAL_NUMBER_OF_PROBES) 
        return;
    
    uint probeIndex = gl_GlobalInvocationID.x % TOTAL_NUMBER_OF_PROBES;
    uint dstTexelCoordX = (gl_GlobalInvocationID.x / TOTAL_NUMBER_OF_PROBES) % SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI;
    uint dstTexelCoordY = gl_GlobalInvocationID.x / (SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI * TOTAL_NUMBER_OF_PROBES);

    ivec2 dstTexelCoordsAdjusted = ivec2(dstTexelCoordX + 1, dstTexelCoordY + 1);
    vec3 dstNormal = oct_to_float32x3(((vec2(dstTexelCoordX, dstTexelCoordY) + vec2(0.5f)) / float(SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI))  * 2.0f - 1.0f); 

    ivec2 dstTextureOffset = ivec2((SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI + 2) * probeIndex, 0);
    vec2 srcTextureOffset = vec2((SAMPLES_PER_PROBE_DIMENSION_GLOSSY_SOURCE + 2) * probeIndex + 1.0f, 1.0f);

    vec3 irradiance = vec3(0.0f);
    float meanDepth = 0.0f;

    float denominator = 0.0f;

    for (uint n = 0; n < SAMPLES_PER_PROBE_GLOSSY_SOURCE; n++)
    {
        vec2 srcTexelCoords = vec2(n % SAMPLES_PER_PROBE_DIMENSION_GLOSSY_SOURCE, n / SAMPLES_PER_PROBE_DIMENSION_GLOSSY_SOURCE) + vec2(0.5f);
        vec3 srcNormal = oct_to_float32x3((srcTexelCoords / float(SAMPLES_PER_PROBE_DIMENSION_GLOSSY_SOURCE)) * 2.0f - 1.0f); //Something wrong here
        float srcDotDst = dot(srcNormal, dstNormal);

        if (srcDotDst > 0.0f)
        {
            float weight = max(0.0f, srcDotDst);
            vec2 offsetSrcTexelCoords = srcTexelCoords + srcTextureOffset;
            vec2 srcIrradianceTexelCoordsAdjusted = offsetSrcTexelCoords / textureSize(glossySourceRadianceAtlas, 0);
            vec2 srcDepthTexelCoordsAdjusted = offsetSrcTexelCoords / textureSize(glossySourceDepthAtlas, 0);

            vec3 radiance = texture(glossySourceRadianceAtlas, srcIrradianceTexelCoordsAdjusted).rgb;
            float depth = texture(glossySourceDepthAtlas, srcDepthTexelCoordsAdjusted).x;

            irradiance += radiance/* * weight */;
            meanDepth += depth/* * weight */;
            denominator += 1.0f;
        }
    }

    irradiance /= denominator;
    //meanDepth /= denominator;
    meanDepth = texture(glossySourceDepthAtlas, vec2(dstTexelCoordX, dstTexelCoordY) + srcTextureOffset).r;

    vec4 collapsedIrradiance = vec4(irradiance, 1.0f);
    vec4 colapsedMeanDepths = vec4(meanDepth, meanDepth * meanDepth, 1.0f, 1.0f);

    imageStore(collapsedGIIrradianceAtlas, dstTexelCoordsAdjusted + dstTextureOffset, collapsedIrradiance);
    imageStore(collapsedGIDepthAtlas, dstTexelCoordsAdjusted + dstTextureOffset, colapsedMeanDepths);

    //Fix Gutters
    {
        if (dstTexelCoordX == 0)
        {
            //Vertical Left Edge, Not Corner
            imageStore(collapsedGIIrradianceAtlas, ivec2(0, SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI - dstTexelCoordY) + dstTextureOffset, collapsedIrradiance);
            imageStore(collapsedGIDepthAtlas, ivec2(0, SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI - dstTexelCoordY) + dstTextureOffset, colapsedMeanDepths);
            
            if (dstTexelCoordY == 0)
            {
                //Bottom Left Corner - Updates Top Right Corner
                imageStore(collapsedGIIrradianceAtlas, ivec2(1 + SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI, 1 + SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI) + dstTextureOffset, collapsedIrradiance);
                imageStore(collapsedGIDepthAtlas, ivec2(1 + SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI, 1 + SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI) + dstTextureOffset, colapsedMeanDepths);
            }
            else if (dstTexelCoordY == SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI - 1)
            {
                //Top Left Corner - Updates Bottom Right Corner
                imageStore(collapsedGIIrradianceAtlas, ivec2(1 + SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI, 0) + dstTextureOffset, collapsedIrradiance);
                imageStore(collapsedGIDepthAtlas, ivec2(1 + SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI, 0) + dstTextureOffset, colapsedMeanDepths);
            }
        }
        else if (dstTexelCoordX == SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI - 1)
        {
            
            //Vertical Right Edge, Not Corner
            imageStore(collapsedGIIrradianceAtlas, ivec2(1 + SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI, SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI - dstTexelCoordY) + dstTextureOffset, collapsedIrradiance);
            imageStore(collapsedGIDepthAtlas, ivec2(1 + SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI, SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI - dstTexelCoordY) + dstTextureOffset, colapsedMeanDepths);
            
            if (dstTexelCoordY == 0)
            {
                //Bottom Right Corner - Updates Top Left Corner
                imageStore(collapsedGIIrradianceAtlas, ivec2(0, 1 + SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI) + dstTextureOffset, collapsedIrradiance);
                imageStore(collapsedGIDepthAtlas, ivec2(0, 1 + SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI) + dstTextureOffset, colapsedMeanDepths);
            }
            else if (dstTexelCoordY == SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI - 1)
            {
                //Top Right Corner - Updates Bottom Left Corner
                imageStore(collapsedGIIrradianceAtlas, ivec2(0, 0) + dstTextureOffset, collapsedIrradiance);
                imageStore(collapsedGIDepthAtlas, ivec2(0, 0) + dstTextureOffset, colapsedMeanDepths);
            }
        }

        if (dstTexelCoordY == 0)
        {
            //Horizontal Bottom Edge
            imageStore(collapsedGIIrradianceAtlas, ivec2(SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI - dstTexelCoordX, 0) + dstTextureOffset, collapsedIrradiance);
            imageStore(collapsedGIDepthAtlas, ivec2(SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI - dstTexelCoordX, 0) + dstTextureOffset, colapsedMeanDepths);
        }
        else if (dstTexelCoordY == SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI - 1)
        {
            //Horizontal Top Edge
            imageStore(collapsedGIIrradianceAtlas, ivec2(SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI - dstTexelCoordX, 1 + SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI) + dstTextureOffset, collapsedIrradiance);
            imageStore(collapsedGIDepthAtlas, ivec2(SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI - dstTexelCoordX, 1 + SAMPLES_PER_PROBE_DIMENSION_COLLAPSED_GI) + dstTextureOffset, colapsedMeanDepths);
        }
    }
}