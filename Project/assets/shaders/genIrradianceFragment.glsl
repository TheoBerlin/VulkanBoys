#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in     vec3 in_Position;
layout(location = 0) out    vec4 out_Color;

layout(binding = 1) uniform samplerCube u_EnvMap; 

const float PI = 3.14159265359f;

void main()
{
    vec3 normal     = normalize(in_Position);
    vec3 irradiance = vec3(0.0f);

    vec3 up     = vec3(0.0f, 1.0f, 0.0f);
    vec3 right  = cross(up, normal);
    up          = cross(normal, right);

    float sampleCount = 0.0f; 
    const float perSample = 0.025f;
    for (float phi = 0.0f; phi < 2.0f * PI; phi += perSample)
    {
        for (float theta = 0.0f; theta < 0.5f * PI; theta += perSample)
        {
            vec3 tangentVector  = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 texCoord       = (tangentVector.x * right) + (tangentVector.y * up) + (tangentVector.z * normal);
            
            irradiance  += texture(u_EnvMap, texCoord).rgb * cos(theta) * sin(theta);
            sampleCount += 1.0f;
        }   
    }

    vec3 finalIrradiance = PI * irradiance * (1.0f / sampleCount); 
    out_Color = vec4(finalIrradiance, 1.0f);
}