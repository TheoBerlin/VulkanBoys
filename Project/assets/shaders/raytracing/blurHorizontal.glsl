#version 460

#include "../helpers.glsl"

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, set = 0, rgb10_a2) uniform image2D u_BlurResultImage;
layout(binding = 1, set = 0) uniform sampler2D u_RawRayTracedResult;
layout(binding = 2, set = 0) uniform sampler2D u_Albedo_AO;
layout(binding = 3, set = 0) uniform sampler2D u_Normal_Roughness;
layout(binding = 4, set = 0) uniform sampler2D u_Depth;

void main()
{
    ivec2 BLUR_IMAGE_SIZE = imageSize(u_BlurResultImage);
    int BLUR_IMAGE_TOTAL_NUM_PIXELS = BLUR_IMAGE_SIZE.x * BLUR_IMAGE_SIZE.y;

    if (gl_GlobalInvocationID.x >= BLUR_IMAGE_TOTAL_NUM_PIXELS) 
        return;

    ivec2 dstPixelCoords = ivec2(gl_GlobalInvocationID.x % BLUR_IMAGE_SIZE.x, gl_GlobalInvocationID.x / BLUR_IMAGE_SIZE.x);
    vec2 texCoords = vec2(dstPixelCoords) / vec2(BLUR_IMAGE_SIZE);

    vec4 centerColor = texture(u_RawRayTracedResult, texCoords);

    if (centerColor.a > 0.0f)
    {
        float roughness = texture(u_Normal_Roughness, texCoords).a;
        float depth = texture(u_Depth, texCoords).r;
        float modifiedDepth = pow(LinearizeDepth(depth, 0.01f, 100.0f), 3.0f);
        float factor = roughness * (1.0f - modifiedDepth);
        vec4 blurColor = blur(u_RawRayTracedResult, centerColor, texCoords, vec2(1.0f, 0.0f), factor);
        imageStore(u_BlurResultImage, dstPixelCoords, vec4(blurColor.rgb, factor));
    }
    else
    {
        imageStore(u_BlurResultImage, dstPixelCoords, vec4(0.0f));
    }
}