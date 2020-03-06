#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MAX_POINT_LIGHTS 4

layout(location = 0) in vec3 in_Normal;
layout(location = 1) in vec3 in_Tangent;
layout(location = 2) in vec3 in_Bitangent;
layout(location = 3) in vec2 in_TexCoord;
layout(location = 4) in vec3 in_WorldPosition;

layout(location = 0) out vec4 outColor;

layout (push_constant) uniform Constants
{
	mat4 Transform;
	vec4 Color;
} constants;

struct PointLight
{
	vec4 Color;
	vec4 Position;
};

layout (binding = 2) uniform LightBuffer
{
	PointLight lights[MAX_POINT_LIGHTS];
} u_Lights;


layout(binding = 3) uniform sampler2D u_AlbedoMap;
layout(binding = 4) uniform sampler2D u_NormalMap;

void main()
{
	vec3 worldPosition = in_WorldPosition;
	vec3 normal 	= normalize(in_Normal);
	vec3 tangent 	= normalize(in_Tangent);
	vec3 bitangent	= normalize(in_Bitangent);
	vec2 texcoord 	= in_TexCoord;
	
	mat3 tbn = mat3(tangent, bitangent, normal);
	
	vec4 texColor 	= texture(u_AlbedoMap, texcoord);
	vec4 normalMap 	= texture(u_NormalMap, texcoord);
	
	vec3 sampledNormal = ((normalMap.xyz * 2.0f) - 1.0f);
	sampledNormal = normalize(sampledNormal);
	sampledNormal = normalize(tbn * sampledNormal);
	
	vec3 finalLightColor = vec3(0.0f);
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		vec3 lightPosition 	= u_Lights.lights[i].Position.xyz;
		vec3 lightColor 	= u_Lights.lights[i].Color.rgb;
		
		vec3 direction = (lightPosition - worldPosition);
		float distance = length(direction);
		float attenuation = 1.0f / (distance * distance);

		direction = normalize(direction);
		float lightStrength = max(dot(direction, sampledNormal), 0.0f);
		
		finalLightColor += lightColor * lightStrength * attenuation;
	}
	
	vec3 color 		= constants.Color.rgb * texColor.rgb;
	vec3 diffuse 	= color * finalLightColor;
	vec3 ambient 	= color * (0.1f);
	vec3 finalColor = min(diffuse + ambient, vec3(1.0f));
    outColor = vec4(finalColor, 1.0f);
}