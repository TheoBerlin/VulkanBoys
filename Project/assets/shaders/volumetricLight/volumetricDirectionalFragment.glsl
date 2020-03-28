#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.1415926535897932384626433832795

layout (push_constant) uniform PushConstants
{
	vec2 viewportExtent;
	uint raymarchSteps;
} g_PushConstants;

layout (binding = 2, set = 0) uniform CameraMatrices
{
	mat4 Projection;
	mat4 View;
	mat4 LastProjection;
	mat4 LastView;
	mat4 InvView;
	mat4 InvProjection;
	vec4 Position;
	vec4 Right;
	vec4 Up;
} g_Camera;

layout (binding = 3, set = 0) uniform sampler2D u_DepthBuffer;

layout (binding = 9, set = 1) uniform DirectionalLight
{
	mat4 viewProj, invViewProj;
	vec4 direction, color;
    float scatterAmount, particleG;
} u_DirectionalLight;

layout (binding = 10, set = 1) uniform sampler2D u_ShadowMap;

layout (location = 0) in vec2 in_TexCoord;

layout (location = 0) out vec4 outColor;

vec3 getRayDestination()
{
	float depth = texture(u_DepthBuffer, in_TexCoord).r;

	vec4 clipPos = vec4(in_TexCoord * 2.0 - vec2(1.0), depth, 1.0);
	vec4 viewPos = g_Camera.InvProjection * clipPos;
	viewPos /= viewPos.w;

	vec4 worldPos = g_Camera.InvView * viewPos;
	return worldPos.xyz;
}

// If visible: 1.0, if in shadow: 0.0
float getShadowFactor(vec3 worldPos)
{
	vec4 lightClipPos = u_DirectionalLight.viewProj * vec4(worldPos, 1.0);
	lightClipPos.xyz /= lightClipPos.w;

	vec2 shadowMapTexCoord = lightClipPos.xy * 0.5 + 0.5;
	float sampledDepth = texture(u_ShadowMap, shadowMapTexCoord).r;

	return sampledDepth > lightClipPos.z ? 1.0 : 0.0;
	//return float(sampledDepth > lightClipPos.z);
}

float phaseFunction(vec3 rayDir)
{
	float g = u_DirectionalLight.particleG;
	float viewAngle = dot(rayDir, -u_DirectionalLight.direction.xyz);

	// Henyey-Greenstein phase function - an approximation of Mie scattering
	float numerator = (1 - g) * (1 - g);
	float denominator = 4 * PI * pow(1 + g * g - 2.0 * g * cos(viewAngle), 1.5);

	return numerator / denominator;
}

vec3 calculateLight(vec3 position, vec3 rayDir)
{
	return getShadowFactor(position) * u_DirectionalLight.color.xyz * phaseFunction(rayDir) * u_DirectionalLight.scatterAmount;
}

void main()
{
	vec3 rayDestination = getRayDestination();
	vec3 rayOrigin = g_Camera.Position.xyz;
    vec3 rayDir = normalize(rayDestination - rayOrigin);

	// Ray-march section
	vec3 raymarchStep = rayDir * length(rayDestination - rayOrigin) / g_PushConstants.raymarchSteps;

	vec3 light = vec3(0);
	//vec3 worldPos = rayOrigin;
	vec3 worldPos = rayDestination;

	for (uint stepNr = 0; stepNr < g_PushConstants.raymarchSteps; stepNr++) {
		light += calculateLight(worldPos, rayDir);

		if (getShadowFactor(worldPos) < 1.0) {
			vec4 lightClipPos = u_DirectionalLight.viewProj * vec4(worldPos, 1.0);
			lightClipPos.xyz /= lightClipPos.w;

			vec2 shadowMapTexCoord = lightClipPos.xy * 0.5 + 0.5;
			float sampledDepth = texture(u_ShadowMap, shadowMapTexCoord).r;

			outColor = vec4(sampledDepth, shadowMapTexCoord.x, lightClipPos.z, 1.0);
			return;
		}

		worldPos -= raymarchStep;
		//worldPos += raymarchStep;
	}

	light /= g_PushConstants.raymarchSteps;
    outColor = vec4(light, 1.0);
}
