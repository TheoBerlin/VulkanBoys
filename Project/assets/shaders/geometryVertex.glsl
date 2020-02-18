#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Vertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
	vec4 TexCoord;
};

layout(location = 0) out vec3 out_Normal;
layout(location = 1) out vec3 out_Tangent;
layout(location = 2) out vec3 out_Bitangent;
layout(location = 3) out vec2 out_TexCoord;
layout(location = 4) out vec3 out_WorldPosition;

layout (push_constant) uniform Constants
{
	mat4 Transform;
	vec4 Color;
	float Ambient;
	float Metallic;
	float Roughness;
} constants;

layout (binding = 0) uniform PerFrameBuffer
{
	mat4 Projection;
	mat4 View;
	vec4 Position;
} g_PerFrame;

layout(binding = 1) buffer vertexBuffer
{ 
	Vertex vertices[];
};

void main() 
{
	vec3 position 		= vertices[gl_VertexIndex].Position.xyz;
    vec3 normal 		= vertices[gl_VertexIndex].Normal.xyz;
	vec3 tangent 		= vertices[gl_VertexIndex].Tangent.xyz;
	vec4 worldPosition 	= constants.Transform * vec4(position, 1.0);
	
	normal 	= normalize((constants.Transform * vec4(normal, 0.0)).xyz);
	tangent = normalize((constants.Transform * vec4(tangent, 0.0)).xyz);
	
	vec3 bitangent 	= normalize(cross(normal, tangent));
	vec2 texCoord 	= vertices[gl_VertexIndex].TexCoord.xy;
	
	out_Normal 			= normal;
	out_Tangent 		= tangent;
	out_Bitangent 		= bitangent;
	out_TexCoord 		= texCoord;
	out_WorldPosition 	= worldPosition.xyz;
	gl_Position = g_PerFrame.Projection * g_PerFrame.View * worldPosition;
}