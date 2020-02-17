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

layout (binding = 1) uniform PerFrameBuffer
{
	mat4 View;
	mat4 Projection;
} g_PerFrame;

layout(binding = 2) uniform CameraProperties 
{
	vec4 up;
	vec4 right;
} g_Camera;

layout (binding = 3) uniform PerParticleBuffer
{
	vec4 position;
} g_PerParticle;

layout (binding = 4) uniform PerEmitterBuffer
{
	vec2 particleSize;
} g_PerEmitter;

layout(location = 0) out vec3 out_Normal;

void main()
{
	vec2 vertexPosition = vertices[gl_VertexIndex].Position.xy;

	vec3 worldPosition = g_PerParticle.position + 
		g_Camera.right 	* vertexPosition.x * g_PerEmitter.particleSize.x +
		g_Camera.up 	* vertexPosition.y * g_PerEmitter.particleSize.y;

	out_Normal = vec3(0.0, 0.0, -1.0);
	gl_Position = g_PerFrame.Projection * g_PerFrame.View * vec4(worldPosition, 1.0);
}
