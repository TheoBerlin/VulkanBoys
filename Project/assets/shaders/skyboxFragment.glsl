#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_TexCoord;

layout(location = 0) out vec4 out_Albedo;
layout(location = 1) out vec4 out_Normal;

layout(binding = 1) uniform samplerCube u_Skybox;

void main()
{
	out_Albedo = texture(u_Skybox, in_TexCoord);
	out_Normal = vec4(0.0f, 0.0f, 0.0f, 0.0f);
}