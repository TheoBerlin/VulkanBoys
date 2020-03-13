#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_Normal;
layout(location = 1) in vec3 in_Tangent;
layout(location = 2) in vec3 in_Bitangent;
layout(location = 3) in vec2 in_TexCoord;

layout(location = 0) out vec4 out_Albedo;
layout(location = 1) out vec4 out_Normals;

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

const float GAMMA = 2.2f;

void main()
{
	vec3 normal 	= normalize(in_Normal);
	vec3 tangent 	= normalize(in_Tangent);
	vec3 bitangent	= normalize(in_Bitangent);
	vec2 texcoord 	= in_TexCoord;
	
	mat3 tbn = mat3(tangent, bitangent, normal);
	
	vec3 texColor 	= pow(texture(u_AlbedoMap, texcoord).rgb, vec3(GAMMA));
	vec3 normalMap 	= texture(u_NormalMap, texcoord).rgb;
	float ao 		= texture(u_AmbientOcclusionMap, texcoord).r;
	float metallic 	= texture(u_MetallicMap, texcoord).r;
	float roughness = texture(u_RoughnessMap, texcoord).r;
	
	vec3 sampledNormal 	= ((normalMap * 2.0f) - 1.0f);
	sampledNormal 		= normalize(tbn * normalize(sampledNormal));
	
	//Store normal in 2 component x^2 + y^2 + z^2 = 1, store the sign with metallic 
	vec2 storedNormal 	= sampledNormal.xy;
	metallic 			= max(constants.Metallic * metallic, 0.00001f);
	if (sampledNormal.z < 0)
	{
		metallic = -metallic;
	}

	out_Albedo 		= vec4(constants.Color.rgb * texColor, constants.Ambient * ao);
	out_Normals		= vec4(storedNormal, metallic, constants.Roughness * roughness);
}