#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_Normal;
layout(location = 1) in vec3 in_Tangent;
layout(location = 2) in vec3 in_Bitangent;
layout(location = 3) in vec2 in_TexCoord;
layout(location = 4) in vec3 in_WorldPosition;

layout(location = 0) out vec4 out_Albedo;
layout(location = 1) out vec4 out_Normals;
layout(location = 2) out vec4 out_Position;

layout (push_constant) uniform Constants
{
	mat4 Transform;
	vec4 Color;
	float Ambient;
	float Metallic;
	float Roughness;
} constants;

layout(binding = 2) uniform sampler2D u_AlbedoMap;
layout(binding = 3) uniform sampler2D u_NormalMap;
layout(binding = 4) uniform sampler2D u_AmbientOcclusionMap;
layout(binding = 5) uniform sampler2D u_MetallicMap;
layout(binding = 6) uniform sampler2D u_RoughnessMap;

void main()
{
	vec3 worldPosition = in_WorldPosition;
	vec3 normal 	= normalize(in_Normal);
	vec3 tangent 	= normalize(in_Tangent);
	vec3 bitangent	= normalize(in_Bitangent);
	vec2 texcoord 	= in_TexCoord;
	
	mat3 tbn = mat3(tangent, bitangent, normal);
	
	vec4 texColor 		= texture(u_AlbedoMap, texcoord);
	vec4 normalMap 		= texture(u_NormalMap, texcoord);
	vec4 aoMap 			= texture(u_AmbientOcclusionMap, texcoord);
	vec4 metallicMap 	= texture(u_MetallicMap, texcoord);
	vec4 roughnessMap 	= texture(u_RoughnessMap, texcoord);
	
	vec3 sampledNormal = ((normalMap.xyz * 2.0f) - 1.0f);
	sampledNormal = normalize(tbn * normalize(sampledNormal));
		
	out_Albedo 		= vec4(constants.Color.rgb * texColor.rgb, constants.Ambient * aoMap.r);
	out_Normals		= vec4(sampledNormal, constants.Metallic * metallicMap.r);
	out_Position 	= vec4(worldPosition, constants.Roughness * roughnessMap.r);
}