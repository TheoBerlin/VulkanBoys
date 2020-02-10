#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Vertex
{
	vec4 Position;
	vec3 Normal;
	vec3 Tangent;
	vec2 TexCoord;
};

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

layout(binding = 1) buffer vertexBuffer { Vertex vertices[]; };

void main() 
{
    gl_Position = g_PerFrame.Projection * g_PerFrame.View * g_Constants.Transform * vec4(vertices[gl_VertexIndex].Position.xyz, 1.0);
}