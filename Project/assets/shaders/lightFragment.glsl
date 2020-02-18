#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MAX_POINT_LIGHTS 4

layout(location = 0) in vec2	in_TexCoord;
layout(location = 0) out vec4	out_Color;

layout(binding = 1) uniform sampler2D u_Albedo;
layout(binding = 2) uniform sampler2D u_Normal;
layout(binding = 3) uniform sampler2D u_WorldPosition;

struct PointLight
{
	vec4 Color;
	vec4 Position;
};

layout (binding = 0) uniform PerFrameBuffer
{
	mat4 Projection;
	mat4 View;
	vec4 Position;
} g_PerFrame;

layout (binding = 4) uniform LightBuffer
{
	PointLight lights[MAX_POINT_LIGHTS];
} u_Lights;

const float PI 					= 3.14159265359f;
const float AMBIENT_LIGHT		= 0.1f;
const float SPECULAR_STRENGTH	= 0.5f;
const int 	SPECULAR_EXPONENT	= 128;
const float GAMMA				= 2.2f;

/*
	Schlick Fresnel function
*/
vec3 Fresnel(vec3 F0, float cosTheta)
{
	return F0 + ((vec3(1.0f) - F0) * pow(1.0f - cosTheta, 5.0f));
}

/*
	GGX Distrubution function
*/
float Distrubution(vec3 normal, vec3 halfVector, float roughness)
{
	float roughnessSqrd = roughness*roughness;
	float alphaSqrd 	= roughnessSqrd *roughnessSqrd;
	
	float NdotH = max(dot(normal, halfVector), 0.0f);
	
	float denom = ((NdotH * NdotH) * (alphaSqrd - 1.0f)) + 1.0f;
	return alphaSqrd / (PI * denom * denom);
}

/*
	Schlick and GGX Geometry-Function
*/
float GeometryGGX(float NdotV, float roughness)
{
	float r 		= roughness + 1.0f;
	float k_Direct 	= (r*r) / 8.0f;
	
	return NdotV / ((NdotV * (1.0f - k_Direct)) + k_Direct);
}

/*
	Smith Geometry-Function
*/
float Geometry(vec3 normal, vec3 viewDir, vec3 lightDirection, float roughness)
{
	float NdotV = max(dot(normal, viewDir), 0.0f);
	float NdotL = max(dot(normal, lightDirection), 0.0f);
	
	return GeometryGGX(NdotV, roughness) * GeometryGGX(NdotL, roughness);
}

void main()
{
	vec2 texCoord = in_TexCoord;

	//Unpack
	vec4 sampledAlbedo 			= texture(u_Albedo, texCoord);
	vec4 sampledNormal 			= texture(u_Normal, texCoord);
	vec4 sampledWorldPosition 	= texture(u_WorldPosition, texCoord);
	vec3 albedo 		= sampledAlbedo.rgb;
	vec3 normal 		= sampledNormal.xyz;
	vec3 worldPosition 	= sampledWorldPosition.xyz;
	
	if (length(worldPosition) <= 0)
	{
		out_Color = vec4(albedo, 1.0f);
		return;
	}
	
	float ao 			= sampledAlbedo.a;
	float metallic 		= sampledNormal.a;
	float roughness 	= sampledWorldPosition.a;
	
	normal = normalize(normal);
	
	vec3 cameraPosition = g_PerFrame.Position.xyz;
	vec3 viewDir 		= normalize(cameraPosition - worldPosition);
	
	vec3 f0 = vec3(0.04f);
	f0 = mix(f0, albedo, metallic);
	
	vec3 L0 = vec3(0.0f);
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		vec3 lightPosition 	= u_Lights.lights[i].Position.xyz;
		vec3 lightColor 	= u_Lights.lights[i].Color.rgb;
		
		vec3 lightDirection = (lightPosition - worldPosition);
		float distance 		= length(lightDirection);
		float attenuation 	= 1.0f / (distance * distance);

		lightDirection 	= normalize(lightDirection);
		vec3 halfVector = normalize(viewDir + lightDirection);
		
		vec3 radiance = lightColor * attenuation;
		
		vec3 f 		= Fresnel(f0, clamp(dot(halfVector, viewDir), 0.0f, 1.0f));	
		float ndf 	= Distrubution(normal, halfVector, roughness);
		float g 	= Geometry(normal, viewDir, lightDirection, roughness);
		
		float denom 	= 4.0f * max(dot(normal, viewDir), 0.0f) * max(dot(normal, lightDirection), 0.0f);
		vec3 specular   = (ndf * g * f) / max(denom, 0.001f);	
		
		//Take 1.0f minus the incoming radiance to get the diffuse (Energy conservation)
		float metallicFactor 	= 1.0f - metallic;
		vec3 diffuse 			= (vec3(1.0f) - f) * metallicFactor;
		
		L0 += ((diffuse * (albedo / PI)) + specular) * radiance * max(dot(normal, lightDirection), 0.0f);
	}
	
	vec3 ambient = vec3(0.03f) * albedo * ao;
	vec3 finalColor = ambient + L0;
	
	finalColor = finalColor / (finalColor + vec3(1.0f));
	finalColor = pow(finalColor, vec3(1.0f / GAMMA)); 
	
    out_Color = vec4(finalColor, 1.0f);
}