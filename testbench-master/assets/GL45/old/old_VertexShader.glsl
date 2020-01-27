#version 440

// from include IA.h
#define POSITION 0
#define NORMAL 1
#define TEXTCOORD 2

layout(location = POSITION) in vec3 vertex_position;
layout(location = NORMAL) in vec3 vertex_color;
layout(location = TEXTCOORD) in vec2 uv;

out vec3 color;

void main() {
	color = vertex_color;
	gl_Position = vec4(vertex_position, 1.0);
}
