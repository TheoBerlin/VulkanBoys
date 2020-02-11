#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Vertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
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

layout(binding = 1) buffer vertexBuffer
{ 
	Vertex vertices[];
};

layout(location = 0) out vec3 out_Normal;

void main() 
{
	vec3 position 	= vertices[gl_VertexIndex].Position.xyz;
    vec3 normal 	= vertices[gl_VertexIndex].Normal.xyz;
	
	out_Normal = normal;
	gl_Position = g_PerFrame.Projection * g_PerFrame.View * g_Constants.Transform * vec4(position, 1.0);
}