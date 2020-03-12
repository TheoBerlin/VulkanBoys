#version 460
#extension GL_NV_ray_tracing : require

#include "../helpers.glsl"

layout(binding = 0, set = 0, rgb10_a2) uniform image2D u_RadianceImage;
layout(binding = 19, set = 0, rgb10_a2) uniform image2D u_ReflectionImage;
layout(binding = 1, set = 0) uniform CameraProperties 
{
	mat4 viewInverse;
	mat4 projInverse;
} u_Cam;
layout(binding = 2, set = 0) uniform accelerationStructureNV u_TopLevelAS;
layout(binding = 3, set = 0) uniform sampler2D u_Albedo_AO;
layout(binding = 4, set = 0) uniform sampler2D u_Normal_Roughness;
layout(binding = 5, set = 0) uniform sampler2D u_Depth;
layout(binding = 17, set = 0) uniform sampler2D u_World_Metallic;
layout(binding = 18, set = 0) uniform sampler2D u_BrdfLUT;
layout(binding = 20, set = 0) uniform sampler2D u_BlueNoiseLUT;

layout (constant_id = 2) const int MAX_POINT_LIGHTS = 4;

struct PointLight
{
	vec4 Color;
	vec4 Position;
};

layout (binding = 16) uniform LightBuffer
{
	PointLight lights[MAX_POINT_LIGHTS];
} u_Lights;

struct RayPayload 
{
	vec3 Radiance;
	uint Recursion;
};

struct ShadowRayPayload
{
	float occluderFactor;
};

layout (push_constant) uniform PushConstants
{
	float counter;
} u_PushConstants;

layout(location = 0) rayPayloadNV RayPayload rayPayload;
layout(location = 1) rayPayloadNV ShadowRayPayload shadowRayPayload;

void calculateHitPosition(in vec2 uvCoords, in float z, out vec3 hitPos, out vec3 viewSpacePos)
{
	vec2 xy = uvCoords * 2.0f - 1.0f;

	vec4 clipSpacePosition = vec4(xy, z, 1.0f);
	vec4 viewSpacePosition = u_Cam.projInverse * clipSpacePosition;
	viewSpacePosition = viewSpacePosition / viewSpacePosition.w;
	vec4 homogenousPosition = u_Cam.viewInverse * viewSpacePosition;

	hitPos = homogenousPosition.xyz;
	viewSpacePos = viewSpacePosition.xyz;
}

void calculateDirections(in vec2 uvCoords, in vec3 hitPosition, in vec3 normal, out vec3 reflDir, out vec3 viewDir)
{
	vec4 u_CameraOrigin = u_Cam.viewInverse * vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec3 origDirection = normalize(hitPosition - u_CameraOrigin.xyz);

	reflDir = reflect(origDirection, normal);
	viewDir = -origDirection;
}

void main() 
{
	//Calculate Screen Coords
	const vec2 pixelCenter = vec2(gl_LaunchIDNV.xy) + vec2(0.5f);
	vec2 uvCoords = (pixelCenter / vec2(gl_LaunchSizeNV.xy));

	//Sample GBuffer
	vec4 sampledNormalRoughness = texture(u_Normal_Roughness, uvCoords);
	vec3 normal = sampledNormalRoughness.xyz;

	//Skybox
	if (dot(normal, normal) < 0.5f)
	{
		imageStore(u_RadianceImage, ivec2(gl_LaunchIDNV.xy), vec4(1.0f, 0.0f, 1.0f, 0.0f));
		imageStore(u_ReflectionImage, ivec2(gl_LaunchIDNV.xy), vec4(1.0f, 0.0f, 1.0f, 0.0f));
		return;
	}

	//Sample GBuffer
	vec4 sampledAlbedoAO = texture(u_Albedo_AO, uvCoords);
	float sampledDepth = texture(u_Depth, uvCoords).r;
	float sampledMetallic = texture(u_World_Metallic, uvCoords).a;

	//Define Constants
	vec3 hitPos = vec3(0.0f);
	vec3 viewSpacePos = vec3(0.0f);
	calculateHitPosition(uvCoords, sampledDepth, hitPos, viewSpacePos);

	//Define new Rays Parameters
	vec3 reflectedRaysOrigin = hitPos + normal * EPSILON;
	uint rayFlags = gl_RayFlagsOpaqueNV;
	uint cullMask = 0xff;
	float tmin = 0.001f;
	float tmax = 10000.0f;

	vec3 reflDir = vec3(0.0f);
	vec3 viewDir = vec3(0.0f);
	calculateDirections(uvCoords, hitPos, normal, reflDir, viewDir);

	//Setup PBR Parameters
	vec3 albedo = sampledAlbedoAO.rgb;
	float ao = sampledAlbedoAO.a;
	float roughness = sampledNormalRoughness.a;
	float metallic = sampledMetallic;

	//Init BRDF values
	vec3 f0 = vec3(0.04f);
	f0 = mix(f0, albedo, metallic);

	float metallicFactor = 1.0f - metallic;
	
	float NdotV = max(dot(viewDir, normal), 0.0f);

	vec3 Lo = vec3(0.0f);
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		vec3 lightPosition 	= u_Lights.lights[i].Position.xyz;
		vec3 lightColor 	= u_Lights.lights[i].Color.rgb;

		vec3 lightVector 	= (lightPosition - hitPos);
		vec3 lightDir 		= normalize(lightVector);

		traceNV(u_TopLevelAS, rayFlags, cullMask, 1, 0, 1, reflectedRaysOrigin, tmin, lightDir, tmax, 1);

		if (shadowRayPayload.occluderFactor < 0.1f)
		{
			float lightDistance	= length(lightVector);
			float attenuation 	= 1.0f / (lightDistance * lightDistance);

			vec3 halfVector = normalize(viewDir + lightDir);

			vec3 radiance = lightColor * attenuation;

			float HdotV = max(dot(halfVector, viewDir), 0.0f);
			float NdotL = max(dot(normal, lightDir), 0.0f);

			vec3 f 		= Fresnel(f0, HdotV);
			float ndf 	= Distribution(normal, halfVector, roughness);
			float g 	= GeometryOpt(NdotV, NdotL, roughness);

			float denom 	= 4.0f * NdotV * NdotL + 0.0001f;
			vec3 specular   = (ndf * g * f) / denom;

			//Take 1.0f minus the incoming radiance to get the diffuse (Energy conservation)
			vec3 diffuse = (vec3(1.0f) - f) * metallicFactor;

			Lo += ((diffuse * (albedo / PI)) + specular) * radiance * NdotL;
		}
	}

	imageStore(u_RadianceImage, ivec2(gl_LaunchIDNV.xy), vec4(Lo, 1.0f));

	//Reflection
	vec2 uniformRandom = texture(u_BlueNoiseLUT, uvCoords).rg;
	reflDir = ReflectanceDirection2(reflDir, roughness, uniformRandom);
	rayPayload.Radiance = vec3(0.0f);
	rayPayload.Recursion = 0;
	traceNV(u_TopLevelAS, rayFlags, cullMask, 0, 0, 0, reflectedRaysOrigin, tmin, reflDir, tmax, 0);

	const float hysterisis = 1.0f;
	vec3 oldRadiance = imageLoad(u_ReflectionImage, ivec2(gl_LaunchIDNV.xy)).rgb;
	vec3 newColor = mix(oldRadiance, rayPayload.Radiance, hysterisis);
	imageStore(u_ReflectionImage, ivec2(gl_LaunchIDNV.xy), vec4(newColor, 1.0f));
}