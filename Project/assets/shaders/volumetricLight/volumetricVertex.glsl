#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Vertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
	vec4 TexCoord;
};

layout(location = 0) out vec3 out_WorldPosition;

layout (push_constant) uniform Constants
{
	mat4 worldMatrix;
} g_Transform;

layout(binding = 0) buffer vertexBuffer
{
	Vertex vertices[];
};

layout (binding = 1) uniform CameraMatrices
{
	mat4 Projection;
	mat4 View;
	mat4 InvView;
	mat4 InvProjection;
	vec4 Position;
} g_CameraMatrices;

void main()
{
	vec4 worldPosition = g_Transform.worldMatrix * vertices[gl_VertexIndex].Position;
	out_WorldPosition = worldPosition.xyz;

	gl_Position = g_CameraMatrices.Projection * g_CameraMatrices.View * worldPosition;
}
