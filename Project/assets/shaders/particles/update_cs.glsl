#version 450

#define TWO_PI 2.0 * 3.1415926535897932384626433832795

layout (push_constant) uniform Constants
{
	float dt;
} g_Time;

layout(binding = 0) buffer Particles
{ 
	vec4 positions[], velocities[];
	float ages[];
} g_Particles;

layout (binding = 1) uniform EmitterProperties
{
	vec4 position, direction;
    vec2 particleSize;
	mat4 centeringRotMatrix;
    float particleDuration, initialSpeed, spread;
	// Used when randomizing a direction for new particles
	float minZ;
} g_EmitterProperties;

float rand(float seed, float minVal, float maxVal)
{
	// noise1 returns a [-1, 1] value
	return (noise1(seed) + 1 + minVal) / (maxVal - minVal);
}

void createParticle(int particleIdx, float particleAge)
{
    // Randomized unit vector within a cone based on https://math.stackexchange.com/a/205589
    float minZ = cos(m_Spread);
    float z = rand(particleAge, g_EmitterProperties.minZ, 1.0);
    float phi = rand(particleAge, 0.0, TWO_PI);

    float sqrtZInv = sqrt(1.0f - z * z);

    // Randomized vector given that the cone is centered around (0,0,1)
    vec3 randVec(sqrtZInv * cos(phi), sqrtZInv * sin(phi), z);
    const vec3 zVec(0.0f, 0.0f, 1.0f);

    vec3 particleDirection;
    if (m_Direction == zVec) {
        particleDirection = randVec;
    } else {
        // Rotate the random vector so that the center of the cone is aligned with the emitter
        particleDirection = g_EmitterProperties.centeringRotMatrix * randVec;
    }

    /*
        a = (0, -g, 0)
        v = (0, -gt, 0) + V0
        p = (0, -(gt^2)/2, 0) + V0*t + P0
    */

    float gt = -9.82 * particleAge;
    vec3 V0 = particleDirection * m_InitialSpeed;
    m_ParticleStorage.velocities[particleIdx] = vec4(vec3(0.0, gt, 0.0) + V0, 0.0);
    m_ParticleStorage.positions[particleIdx] = vec4(vec3(0.0, gt * particleAge / 2.0, 0.0) + V0 * particleAge + m_Position, 1.0);
    m_ParticleStorage.ages[particleIdx] = particleAge;
}

void main()
{
	float dt = g_Time.dt;
	float age = g_Particles.ages[gl_GlobalInvocationID];
	age += dt;

	if (age > g_EmitterProperties.particleDuration) {
		// Respawn old particle
		float newParticleAge = age - g_EmitterProperties.particleDuration;
        createParticle(particleIdx, newParticleAge);
	} else {
		g_Particles.positions[gl_GlobalInvocationID] += g_Particles.velocities[gl_GlobalInvocationID] * dt;
		g_Particles.velocities[gl_GlobalInvocationID].y -= 9.82 * dt;
		g_Particles.ages[gl_GlobalInvocationID] += age;
	}
}
