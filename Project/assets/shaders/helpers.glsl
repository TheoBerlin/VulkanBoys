
const float PI 		= 3.14159265359f;
const float EPSILON = 0.001f;

vec4 bilateralBlur13Roughness(sampler2D image, vec4 centerColor, vec2 texCoords, vec2 direction, float roughness) 
{
    vec3 color = vec3(0.0f);
    vec2 resolution = textureSize(image, 0);
    float roughnessFactor = pow(0.5f + roughness, 2.0f);
    roughnessFactor = 1.0f;
    vec2 off1 = roughnessFactor * vec2(1.411764705882353f)  * direction / resolution;
    vec2 off2 = roughnessFactor * vec2(3.2941176470588234f) * direction / resolution;
    vec2 off3 = roughnessFactor * vec2(5.176470588235294f)  * direction / resolution;

    vec4 color0 = texture(image, texCoords + off1);
    vec4 color1 = texture(image, texCoords - off1);
    vec4 color2 = texture(image, texCoords + off2);
    vec4 color3 = texture(image, texCoords - off2);
    vec4 color4 = texture(image, texCoords + off3);
    vec4 color5 = texture(image, texCoords - off3);

    const float denom = sqrt(3.0f);
    float closenessCoeff0 = 1.0f - distance(centerColor, color0) / denom;
    float closenessCoeff1 = 1.0f - distance(centerColor, color1) / denom;
    float closenessCoeff2 = 1.0f - distance(centerColor, color2) / denom;
    float closenessCoeff3 = 1.0f - distance(centerColor, color3) / denom;
    float closenessCoeff4 = 1.0f - distance(centerColor, color4) / denom;
    float closenessCoeff5 = 1.0f - distance(centerColor, color5) / denom;

    float weight0 = color0.a * closenessCoeff0 * 0.2969069646728344f;
    float weight1 = color1.a * closenessCoeff1 * 0.2969069646728344f;
    float weight2 = color2.a * closenessCoeff2 * 0.09447039785044732f;
    float weight3 = color3.a * closenessCoeff3 * 0.09447039785044732f;
    float weight4 = color4.a * closenessCoeff4 * 0.010381362401148057f;
    float weight5 = color5.a * closenessCoeff5 * 0.010381362401148057f;

    color += centerColor.rgb * 0.1964825501511404f;
    color += color0.rgb * weight0;
    color += color1.rgb * weight1;
    color += color2.rgb * weight2;
    color += color3.rgb * weight3;
    color += color4.rgb * weight4;
    color += color5.rgb * weight5;

    float normalization = 0.0f;
    normalization += weight0;
    normalization += weight1;
    normalization += weight2;
    normalization += weight3;
    normalization += weight4;
    normalization += weight5;

    return vec4(color.rgb, 1.0f) / normalization;
}

/*
	Schlick Fresnel function
*/
vec3 Fresnel(vec3 F0, float cosTheta)
{
	return F0 + ((vec3(1.0f) - F0) * pow(1.0f - cosTheta, 5.0f));
}

/*
	Schlick Fresnel function with roughness
*/
vec3 FresnelRoughness(vec3 F0, float cosTheta, float roughness)
{
	return F0 + ((max(vec3(1.0f - roughness), F0) - F0) * pow(1.0f - cosTheta, 5.0f));
}

/*
	GGX Distribution function
*/
float Distribution(vec3 normal, vec3 halfVector, float roughness)
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

/*
	Smith Geometry-Function
*/
float GeometryOpt(float NdotV, float NdotL, float roughness)
{
	return GeometryGGX(NdotV, roughness) * GeometryGGX(NdotL, roughness);
}

float gold_noise3(vec3 x, float seed, float min, float max)
{
    const float GOLD_PHI = 1.61803398874989484820459 * 00000.1; // Golden Ratio   
    const float GOLD_PI  = 3.14159265358979323846264 * 00000.1; // PI
    const float GOLD_SQ2 = 1.41421356237309504880169 * 10000.0; // Square Root of Two
    const float GOLD_E   = 2.71828182846;

    return mix(min, max, fract(tan(distance(x * (seed + GOLD_PHI), vec3(GOLD_PHI, GOLD_PI, GOLD_E))) * GOLD_SQ2) * 0.5f + 0.5f);
}


void CreateCoordinateSystem(in vec3 N, out vec3 Nt, out vec3 Nb) 
{ 
    if (abs(N.x) > abs(N.y)) 
        Nt = vec3(N.z, 0.0f, -N.x) / sqrt(N.x * N.x + N.z * N.z); 
    else 
        Nt = vec3(0.0f, -N.z, N.y) / sqrt(N.y * N.y + N.z * N.z); 
    Nb = cross(N, Nt); 
}

vec3 ReflectanceDirection(vec3 seed, vec3 reflDir, float roughness)
{
    const float goldseed = 0.0f;
    const vec3 offset1 = vec3(1.0f, 2.0f, 3.0f);
    const vec3 offset2 = vec3(-3.0f, -2.0f, -1.0f);

    float theta = roughness * PI / 8.0f;
    float z = gold_noise3(seed + offset1, goldseed, cos(theta), 1.0f);
    float phi = gold_noise3(seed + offset2, goldseed, 0.0f, 2 * PI);
    float sinTheta = sqrt(1.0f - z * z);

    vec3 coneVector = vec3(sinTheta * cos(phi), sinTheta * sin(phi), z);
    
    float c = dot(coneVector, reflDir);

    // if (1.0f - abs(c) < EPSILON)
    //     return mix(reflDir, coneVector, roughness);

    vec3 v = cross(coneVector, reflDir);
    float s = length(v);

    mat3 Vx =  mat3(vec3( 0.0f,  -v.z,    v.y),
                    vec3( v.z,    0.0f,  -v.x),
                    vec3(-v.y,    v.x,    0.0f));
    mat3 R = mat3(1.0f) + Vx + Vx * Vx / (1.0f + c);

    vec3 result = R * coneVector;
    

    return vec3(-result.x, -result.y, result.z);
}
