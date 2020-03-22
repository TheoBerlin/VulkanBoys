#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Vertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
	vec4 TexCoord;
};

layout(binding = 0) buffer vertexBuffer
{
	Vertex vertices[];
};

layout (binding = 1) uniform VolumetricPointLight
{
	mat4 worldMatrix;

	// Light data
	vec4 lightPosition, lightColor;
	float lightRadius, lightScatterAmount;

	// Determines the portion of forward scattered light
	float particleG;
} g_Light;

layout (binding = 2) uniform CameraMatrices
{
	mat4 Projection;
	mat4 View;
	mat4 InvView;
	mat4 InvProjection;
	vec4 Position;
} g_CameraMatrices;

layout(location = 0) out vec3 out_WorldPosition;

void main()
{
	vec4 vertPos = vec4(vertices[gl_VertexIndex].Position.xyz, 1.0);
	vec4 worldPosition = g_Light.worldMatrix * vertPos;
	out_WorldPosition = worldPosition.xyz;

	gl_Position = g_CameraMatrices.Projection * g_CameraMatrices.View * worldPosition;
}
