// inputs
#ifdef NORMAL
	layout( location = NORMAL ) in vec4 normal_in;
#endif

#ifdef TEXTCOORD
	layout (location = TEXTCOORD ) in vec2 uv_in;
#endif

out vec4 fragment_color;

layout(binding=DIFFUSE_TINT) uniform DIFFUSE_TINT_NAME
{
	vec4 diffuseTint;
};

// binding sets the TEXTURE_UNIT value!
#ifdef DIFFUSE_SLOT
layout(binding=DIFFUSE_SLOT) uniform sampler2D myTex;
#endif

void main () {
	#ifdef DIFFUSE_SLOT
    vec4 col = texture(myTex, uv_in);
	#else
	vec4 col = vec4(1.0,1.0,1.0, 1.0);
	#endif

	fragment_color = col * vec4(diffuseTint.rgb,1.0);

	return;
	
//	#ifdef NORMAL
//		fragment_color = vec4(1.0,1.0,normal_in.z, 1.0) * vec4(diffuseTint.rgb, 1.0);
//	#else
//		fragment_color = vec4(1.0,1.0,1.0,1.0) * vec4(diffuseTint.rgb, 1.0);
//	#endif
}
