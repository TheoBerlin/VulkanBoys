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

layout (push_constant) uniform PushConstants
{
	mat4 worldMatrix;

	// Light data
	vec3 lightPosition, lightColor;
	float lightRadius, lightScatterAmount;
	// Determines the portion of forward scattered light
	float particleG;

	uint raymarchSteps;
} g_PushConstants;

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
	vec4 worldPosition = g_PushConstants.worldMatrix * vertices[gl_VertexIndex].Position;
	out_WorldPosition = worldPosition.xyz;

	gl_Position = g_CameraMatrices.Projection * g_CameraMatrices.View * worldPosition;
}
