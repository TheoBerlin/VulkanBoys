#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (push_constant) uniform Constants
{
	vec4 Color;
} g_Constants;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = g_Constants.Color;
}