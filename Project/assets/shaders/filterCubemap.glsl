#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 out_Position;

const vec3 positions[36] = vec3[]
(
	// Back
	vec3( 1.0f, -1.0f, -1.0f),           
	vec3( 1.0f,  1.0f, -1.0f),  
	vec3(-1.0f, -1.0f, -1.0f), 
	vec3(-1.0f,  1.0f, -1.0f),  
	vec3(-1.0f, -1.0f, -1.0f),  
	vec3( 1.0f,  1.0f, -1.0f),  
	
	// Front
	vec3( 1.0f,  1.0f,  1.0f),  
	vec3( 1.0f, -1.0f,  1.0f),  
	vec3(-1.0f, -1.0f,  1.0f),  
	vec3(-1.0f, -1.0f,  1.0f),  
	vec3(-1.0f,  1.0f,  1.0f),  
	vec3( 1.0f,  1.0f,  1.0f),  

	// Left
	vec3(-1.0f, -1.0f, -1.0f),
	vec3(-1.0f,  1.0f, -1.0f),
	vec3(-1.0f,  1.0f,  1.0f),
	vec3(-1.0f,  1.0f,  1.0f),
	vec3(-1.0f, -1.0f,  1.0f),
	vec3(-1.0f, -1.0f, -1.0f),
	
	// Right
	vec3( 1.0f,  1.0f, -1.0f),        
	vec3( 1.0f, -1.0f, -1.0f),  
	vec3( 1.0f,  1.0f,  1.0f),  
	vec3( 1.0f, -1.0f,  1.0f),      
	vec3( 1.0f,  1.0f,  1.0f),  
	vec3( 1.0f, -1.0f, -1.0f),  
	
	// Bottom
	vec3( 1.0f, -1.0f,  1.0f),  
	vec3( 1.0f, -1.0f, -1.0f),  
	vec3(-1.0f, -1.0f, -1.0f),  
	vec3(-1.0f, -1.0f, -1.0f),  
	vec3(-1.0f, -1.0f,  1.0f),  
	vec3( 1.0f, -1.0f,  1.0f),  
	
	// Top
	vec3( 1.0f,  1.0f, -1.0f),    
	vec3( 1.0f,  1.0f , 1.0f),  
	vec3(-1.0f,  1.0f, -1.0f),  
	vec3(-1.0f,  1.0f,  1.0f),
	vec3(-1.0f,  1.0f, -1.0f),  
	vec3( 1.0f,  1.0f,  1.0f)  
);

layout (binding = 0) uniform Camera 
{ 
	mat4 Projection; 
} camera;

layout (push_constant) uniform Constants
{
	mat4 View;
	float Roughness;
} constants;

void main() 
{
	vec3 position 	= positions[gl_VertexIndex];
	out_Position	= position;
	
	gl_Position 	= camera.Projection * constants.View * vec4(position, 1.0f);
}