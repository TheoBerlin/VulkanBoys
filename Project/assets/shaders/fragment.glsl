#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_Normal;
layout(location = 1) in vec3 in_Tangent;
layout(location = 2) in vec3 in_Bitangent;
layout(location = 3) in vec2 in_TexCoord;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform Constants
{
	mat4 Transform;
	vec4 Color;
} constants;

layout(binding = 2) uniform sampler2D u_Albedo;

const vec3 dir = vec3(0.0f, 1.0f, -1.0f);

void main()
{
	vec3 direction 	= normalize(dir);
	vec3 normal 	= normalize(in_Normal);
	float lightStrength = dot(direction, normal);
	
	vec4 texColor = texture(u_Albedo, in_TexCoord);
    outColor = constants.Color * texColor * max(lightStrength, 0.3f);
}