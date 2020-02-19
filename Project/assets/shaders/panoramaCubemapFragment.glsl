#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in 	vec3 in_Position;
layout(location = 0) out 	vec4 out_Color;

layout(binding = 0) uniform sampler2D u_Panorama;

const vec2 invAtan = vec2(0.1591f, -0.3183f);

vec2 GetTexCoordFromPosition(vec3 position)
{
	vec2 texCoord = vec2(atan(position.z, position.x), asin(position.y));
	texCoord = texCoord * invAtan;
	texCoord += vec2(0.5f);
	
	return texCoord;
}

void main()
{
	vec3 position = normalize(in_Position);
	vec2 texCoord = GetTexCoordFromPosition(position);
	vec3 color = texture(u_Panorama, texCoord).rgb;
	
	out_Color = vec4(color, 1.0f);
}