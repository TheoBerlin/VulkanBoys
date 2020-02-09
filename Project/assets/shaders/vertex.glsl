#version 450
#extension GL_ARB_separate_shader_objects : enable

vec3 positions[3] = vec3[]
(
    vec3(0.0, -0.5, 0.0),
    vec3(0.5, 0.5, 0.0),
    vec3(-0.5, 0.5, 0.0)
);

layout (push_constant) uniform Constants
{
	mat4 Transform;
	vec4 Color;
} g_Constants;

layout (binding = 0) uniform PerFrameBuffer
{
	mat4 Projection;
	mat4 View;
} g_PerFrame;

void main() 
{
    gl_Position = g_PerFrame.Projection * g_PerFrame.View * g_Constants.Transform * vec4(positions[gl_VertexIndex], 1.0);
}