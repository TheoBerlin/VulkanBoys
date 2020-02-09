#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform Constants
{
	mat4 Transform;
	vec4 Color;
} g_Constants;

void main()
{
    outColor = g_Constants.Color;
}