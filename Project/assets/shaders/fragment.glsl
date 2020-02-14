#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform Constants
{
	mat4 Transform;
	vec4 Color;
} g_Constants;

layout(location = 0) in vec3 in_Normal;

const vec3 dir = vec3(0.0f, 1.0f, -1.0f);

void main()
{
	vec3 direction 	= normalize(dir);
	vec3 normal 	= normalize(in_Normal);
	float lightStrength = dot(direction, normal);
	
    outColor = g_Constants.Color * max(lightStrength, 0.3f);
}