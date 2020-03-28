#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 4, set = 0) uniform sampler2D u_VolumetricLightBuffer;

layout (location = 0) in vec2 in_TexCoord;
layout (location = 0) out vec3 outColor;

void main()
{
    outColor = texture(u_VolumetricLightBuffer, in_TexCoord).rgb;
}
