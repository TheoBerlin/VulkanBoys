#version 450
#extension GL_ARB_separate_shader_objects : enable

struct QuadVertex
{
	vec2 Position;
	vec2 TexCoord;
};

layout(binding = 0) buffer vertexBuffer
{
	QuadVertex vertices[];
};

layout (binding = 1) uniform CameraMatrices
{
	//mat4 View;
	//mat4 Projection;
	mat4 Projection;
	mat4 View;
	mat4 LastProjection;
	mat4 LastView;
	mat4 InvView;
	mat4 InvProjection;
	vec4 Position;
	vec4 Right;
	vec4 Up;
} g_CameraMatrices;

/*layout(binding = 2) uniform CameraDirections
{
	vec4 up;
	vec4 right;
} g_CameraDirections;*/

layout (binding = 3) uniform EmitterProperties
{
	mat4 centeringRotMatrix;
	vec4 position, direction;
    vec2 particleSize; // Only this variable is used in this shader
    float particleDuration, initialSpeed, spread, particleCount;
} g_EmitterProperties;

layout (binding = 4) buffer ParticlePositions
{
	vec4 positions[];
} g_ParticlePositions;

layout(location = 0) out vec2 out_TexCoords;

void main()
{
	QuadVertex vertex = vertices[gl_VertexIndex];
	vec2 vertexPosition = vertex.Position.xy;

	vec4 worldPosition = g_ParticlePositions.positions[gl_InstanceIndex] +
		g_CameraMatrices.Right 	* vertexPosition.x * g_EmitterProperties.particleSize.x +
		g_CameraMatrices.Up 	* vertexPosition.y * g_EmitterProperties.particleSize.y;

	out_TexCoords = vertex.TexCoord;
	gl_Position = g_CameraMatrices.Projection * g_CameraMatrices.View * worldPosition;
}
