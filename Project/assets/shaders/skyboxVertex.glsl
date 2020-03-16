#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform CameraBuffer
{
	mat4 Projection;
	mat4 View;
	mat4 LastProjection;
	mat4 LastView;
	mat4 InvView;
	mat4 InvProjection;
	vec4 Position;
} camera;

layout(location = 0) out vec3 out_TexCoord;

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

void main()
{
	mat4 view = mat4(mat3(camera.View));

	out_TexCoord 	= positions[gl_VertexIndex];
	vec4 position 	= camera.Projection * view * vec4(out_TexCoord, 1.0f);
	gl_Position 	= position.xyww;
}