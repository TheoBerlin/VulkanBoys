
vec4 bilateralBlur13Roughness(sampler2D image, vec4 centerColor, vec2 texCoords, vec2 direction, float roughness) 
{
    vec3 color = vec3(0.0f);
    vec2 resolution = textureSize(image, 0);
    float roughnessFactor = pow(0.5f + roughness, 2.0f);
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