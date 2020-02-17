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

const float AMBIENT_LIGHT		= 0.1f;
const float SPECULAR_STRENGTH	= 0.5f;
const int 	SPECULAR_EXPONENT	= 128;

void main()
{
	vec2 texCoord = in_TexCoord;

	vec3 albedo 		= texture(u_Albedo, texCoord).rgb;
	vec3 normal 		= texture(u_Normal, texCoord).xyz;
	vec3 worldPosition 	= texture(u_WorldPosition, texCoord).xyz;
	
	normal = normalize(normal);
	
	vec3 cameraPosition = g_PerFrame.Position.xyz;
	vec3 viewDir 		= normalize(cameraPosition - worldPosition);
	
	vec3 finalSpecular 	 = vec3(0.0f); 
	vec3 finalLightColor = vec3(0.0f);
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		vec3 lightPosition 	= u_Lights.lights[i].Position.xyz;
		vec3 lightColor 	= u_Lights.lights[i].Color.rgb;
		
		vec3 lightDirection = (lightPosition - worldPosition);

		float distance 		= length(lightDirection);
		float attenuation 	= 1.0f / (distance * distance);

		lightDirection = normalize(lightDirection);
		float lightStrength = max(dot(lightDirection, normal), 0.0f);
		
		vec3 reflectDir 	= reflect(-lightDirection, normal);		
		float specular 		= pow(max(dot(viewDir, reflectDir), 0.0f), SPECULAR_EXPONENT);
		
		finalLightColor += lightColor * lightStrength * attenuation;
		finalSpecular 	+= SPECULAR_STRENGTH * specular * lightColor;
	}
	
	vec3 diffuse 	= albedo * finalLightColor;
	vec3 ambient 	= albedo * AMBIENT_LIGHT;
	vec3 specular 	= albedo * finalSpecular;
	vec3 finalColor = min(diffuse + ambient + specular, vec3(1.0f));
    out_Color = vec4(finalColor, 1.0f);
}