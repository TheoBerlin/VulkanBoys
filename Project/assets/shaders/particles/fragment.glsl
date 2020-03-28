#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 4) uniform sampler2D particleTexture;

layout (location = 0) in vec2 texCoords;
layout (location = 0) out vec4 outColor;

void main()
{
    outColor = texture(particleTexture, texCoords);
}
