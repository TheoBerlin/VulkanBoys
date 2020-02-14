#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MAX_POINT_LIGHTS 4

layout(location = 0) in vec2	in_TexCoord;
layout(location = 0) out vec4	out_Color;

layout(binding = 0) uniform sampler2D u_Albedo;
layout(binding = 1) uniform sampler2D u_Normal;
layout(binding = 2) uniform sampler2D u_WorldPosition;

struct PointLight
{
	vec4 Color;
	vec4 Position;
};

layout (binding = 3) uniform LightBuffer
{
	PointLight lights[MAX_POINT_LIGHTS];
} u_Lights;

void main()
{
	vec2 texCoord = in_TexCoord;

	vec3 albedo 		= texture(u_Albedo, texCoord).rgb;
	vec3 normal 		= texture(u_Normal, texCoord).xyz;
	vec3 worldPosition 	= texture(u_WorldPosition, texCoord).xyz;
	
	normal = normalize(normal);
	
	vec3 finalLightColor = vec3(0.0f);
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		vec3 lightPosition 	= u_Lights.lights[i].Position.xyz;
		vec3 lightColor 	= u_Lights.lights[i].Color.rgb;
		
		vec3 direction = (lightPosition - worldPosition);
		float distance = length(direction);
		float attenuation = 1.0f / (distance * distance);

		direction = normalize(direction);
		float lightStrength = max(dot(direction, normal), 0.0f);
		
		finalLightColor += lightColor * lightStrength * attenuation;
	}
	
	vec3 diffuse 	= albedo * finalLightColor;
	vec3 ambient 	= albedo * (0.1f);
	vec3 finalColor = min(diffuse + ambient, vec3(1.0f));
    out_Color = vec4(finalColor, 1.0f);
}