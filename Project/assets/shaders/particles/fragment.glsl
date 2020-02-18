#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec2 texCoord;
//layout(binding = 5) uniform sampler2D texSampler;
//layout(binding = 6, set = 0) uniform sampler2D albedoMaps[16];

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(texCoord, 0.0, 1.0);
}
