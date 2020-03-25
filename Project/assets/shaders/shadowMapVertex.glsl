#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Vertex
{
	vec4 Position;
	vec4 Normal;
	vec4 Tangent;
	vec4 TexCoord;
};

struct InstanceTransforms
{
	mat4 CurrTransform;
	mat4 PrevTransform;
};

layout (push_constant) uniform Constants
{
	int TransformsIndex;
} constants;

layout (binding = 0) uniform PerFrameBuffer
{
	mat4 Projection;
	mat4 View;
	mat4 LastProjection;
	mat4 LastView;
	mat4 InvView;
	mat4 InvProjection;
	vec4 Position;
	vec4 Right;
	vec4 Up;
} g_PerFrame;

layout(binding = 1) buffer vertexBuffer
{
	Vertex vertices[];
};

layout(binding = 8, set = 0) buffer CombinedInstanceTransforms
{
	InstanceTransforms t[];
} u_Transforms;

layout(binding = 9, set = 1) buffer LightTransforms
{
	mat4 viewProj;
    mat4 invViewProj;
} u_LightTransforms;

void main()
{
    vec3 position       = vertices[gl_VertexIndex].Position.xyz;
	mat4 currTransform  = u_Transforms.t[constants.TransformsIndex].CurrTransform;

	vec4 worldPosition  = currTransform * vec4(position, 1.0);
    gl_Position         = u_LightTransforms.viewProj * worldPosition;
}
