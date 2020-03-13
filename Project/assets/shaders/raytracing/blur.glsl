#version 460

#include "../helpers.glsl"

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, set = 0) uniform sampler2D u_InputImage;
layout(binding = 1, set = 0, rgba16f) uniform image2D u_OutputImage;
layout(binding = 2, set = 0) uniform sampler2D u_Albedo_AO;
layout(binding = 3, set = 0) uniform sampler2D u_Normal_Roughness;
layout(binding = 4, set = 0) uniform sampler2D u_Depth;

layout (push_constant) uniform PushConstants
{
	vec2 Direction;
} u_PushConstants;

void main()
{
    ivec2 BLUR_IMAGE_SIZE = imageSize(u_OutputImage);
    int BLUR_IMAGE_TOTAL_NUM_PIXELS = BLUR_IMAGE_SIZE.x * BLUR_IMAGE_SIZE.y;

    if (gl_GlobalInvocationID.x >= BLUR_IMAGE_TOTAL_NUM_PIXELS) 
        return;

    ivec2 dstPixelCoords = ivec2(gl_GlobalInvocationID.x % BLUR_IMAGE_SIZE.x, gl_GlobalInvocationID.x / BLUR_IMAGE_SIZE.x);
    vec2 texCoords = vec2(dstPixelCoords) / vec2(BLUR_IMAGE_SIZE);

    vec4 centerColor = texture(u_InputImage, texCoords);

    if (centerColor.a > 0.01f)
    {
        float roughness = texture(u_Normal_Roughness, texCoords).a;
        float depth = texture(u_Depth, texCoords).r;
        float modifiedDepth = pow(LinearizeDepth(depth, 0.01f, 100.0f), 3.0f);
        float factor = roughness * (1.0f - depth);

        vec4 blurColor = blur(u_InputImage, centerColor, texCoords, u_PushConstants.Direction, factor);
        imageStore(u_OutputImage, dstPixelCoords, vec4(blurColor.rgb, centerColor.a));
    }
    else
    {
        imageStore(u_OutputImage, dstPixelCoords, centerColor);
    }
}