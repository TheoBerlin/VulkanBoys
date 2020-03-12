#version 450

layout (local_size_x_id=1, local_size_y=1, local_size_z=1) in;

#define TWO_PI 2.0 * 3.1415926535897932384626433832795
#define BOUNCE_COEFFICIENT 0.8

layout (push_constant) uniform Constants
{
	float dt;
    bool performCollisions;
} g_PushConstant;

layout (binding = 0) buffer Positions
{
	vec4 positions[];
} g_Positions;

layout (binding = 1) buffer Velocities
{
	vec4 velocities[];
} g_Velocities;

layout (binding = 2) buffer Ages
{
	float ages[];
} g_Ages;

layout (binding = 3) uniform EmitterProperties
{
	mat4 centeringRotMatrix;
	vec4 position, direction;
    vec2 particleSize;
    float particleDuration, initialSpeed, spread, particleCount;
} g_EmitterProperties;

layout (binding = 4) uniform Camera
{
	mat4 View;
	mat4 Projection;
    vec4 Position;
} g_Cam;

layout(binding = 5) uniform sampler2D g_DepthBuffer;
layout(binding = 6) uniform sampler2D g_NormalMap;

float rand1(float p, float minVal, float maxVal)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return minVal + (maxVal - minVal) * fract(p);
}

void createParticle(uint particleIdx, float particleAge)
{
    // Randomized unit vector within a cone based on https://math.stackexchange.com/a/205589
    float minZ = cos(g_EmitterProperties.spread);
    float z = rand1(particleIdx, minZ, 1.0);
    float phi = rand1(particleIdx, 0.0, TWO_PI);

    float sqrtZInv = sqrt(1.0f - z * z);

    // Randomized vector given that the cone is centered around (0,0,1)
    vec3 randVec = vec3(sqrtZInv * cos(phi), sqrtZInv * sin(phi), z);
    const vec3 zVec = vec3(0.0f, 0.0f, 1.0f);

    vec3 particleDirection;
    if (g_EmitterProperties.direction.xyz == zVec) {
        particleDirection = randVec;
    } else {
        // Rotate the random vector so that the center of the cone is aligned with the emitter
        particleDirection = vec3(g_EmitterProperties.centeringRotMatrix * vec4(randVec, 0.0));
    }

    /*
        a = (0, -g, 0)
        v = (0, -gt, 0) + V0
        p = (0, -(gt^2)/2, 0) + V0*t + P0
    */

    float gt = -9.82 * particleAge;
    vec3 V0 = particleDirection * g_EmitterProperties.initialSpeed;

    g_Velocities.velocities[particleIdx] = vec4(vec3(0.0, gt, 0.0) + V0, particleAge);
    g_Positions.positions[particleIdx] = vec4(vec3(0.0, gt * particleAge * 0.5, 0.0) + V0 * particleAge + g_EmitterProperties.position.xyz, 1.0);
    g_Ages.ages[particleIdx] = particleAge;
}

void collide(uint particleIdx)
{
    // Translate particle's world coordinates into screen coordinates
    vec4 worldPos = g_Positions.positions[particleIdx];
    vec4 clipSpace = g_Cam.Projection * g_Cam.View * worldPos;

    vec2 ndc = clipSpace.xy / clipSpace.w;
    if (abs(ndc.x) > 1.0 || abs(ndc.y) > 1.0) {
        // The particle's center is off-screen, abort
        return;
    }

    vec2 screenCoords = ndc * 0.5 + 0.5;

    // Compare camera distance and depth value to decide whether or not the particle is colliding with geometry
    float depthVal = texture(g_DepthBuffer, screenCoords).r;
    float cameraDistance = length(worldPos - g_Cam.Position);

    if (abs(depthVal - cameraDistance) < 0.1) {
        // Handle collision
        vec3 normal = texture(g_NormalMap, screenCoords).xyz;

        g_Velocities.velocities[particleIdx] = vec4(reflect(g_Velocities.velocities[particleIdx].xyz, normal) * BOUNCE_COEFFICIENT, 0.0);
    }
}

void main()
{
    uint particleIdx = gl_GlobalInvocationID.x;
    if (particleIdx >= g_EmitterProperties.particleCount) {
        return;
    }

	float dt = g_PushConstant.dt;
    float age = g_Ages.ages[particleIdx] + dt;

    // Move particle
    g_Positions.positions[particleIdx] += g_Velocities.velocities[particleIdx] * dt;
    g_Velocities.velocities[particleIdx].y -= 9.82 * dt;
    g_Ages.ages[particleIdx] = age;

	if (age > g_EmitterProperties.particleDuration) {
		// Respawn old particle
		float newParticleAge = age - g_EmitterProperties.particleDuration;
        createParticle(particleIdx, newParticleAge);
	}

    if (g_PushConstant.performCollisions) {
        collide(particleIdx);
    }
}
