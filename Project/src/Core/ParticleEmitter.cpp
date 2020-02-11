#include "ParticleEmitter.hpp"

#include "Common/IMesh.h"

ParticleEmitter::ParticleEmitter(glm::vec3 position, glm::vec3 direction, float particleDuration, float initialSpeed, float particlesPerSecond)
    :m_Position(position),
    m_Direction(glm::normalize(direction)),
    m_ParticleDuration(particleDuration),
    m_InitialSpeed(initialSpeed),
    m_ParticlesPerSecond(particlesPerSecond),
    m_EmitterAge(0.0f)
{}

ParticleEmitter::~ParticleEmitter()
{}

void ParticleEmitter::initialize(IGraphicsContext* pGraphicsContext)
{
    IMesh* pParticleMesh = pGraphicsContext->createMesh();
    if (!pParticleMesh->initFromFile("assets/meshes/cube.obj")) {
        LOG("Failed to load particle mesh");
        return;
    }

    size_t particleCount = m_ParticlesPerSecond * m_ParticleDuration;
    m_ParticleStorage.positions.reserve(particleCount);
    m_ParticleStorage.velocities.reserve(particleCount);
    m_ParticleStorage.ages.reserve(particleCount);
}

void ParticleEmitter::update(float dt)
{
    size_t maxParticleCount = m_ParticlesPerSecond * m_ParticleDuration;
    if (m_ParticleStorage.positions.size() < maxParticleCount) {
        m_EmitterAge += dt;
        spawnParticles();
    }

    std::vector<glm::vec3>& positions = m_ParticleStorage.positions;
    std::vector<glm::vec3>& velocities = m_ParticleStorage.velocities;
    std::vector<float>& ages = m_ParticleStorage.ages;

    for (size_t particleIdx = 0; particleIdx < m_ParticleStorage.positions.size(); particleIdx++) {
        positions[particleIdx] += velocities[particleIdx] * dt;
        velocities[particleIdx].y -= 9.82f * dt;
        ages[particleIdx] += dt;
    }

    /*
        TODO: Optimize respawning, dead pixels are grouped up, no need to iterate through the entire particle storage
        Two cases of particles grouping up: (d=dead, a=alive)
        (front) dddaaaddd (back)
        (front) aaaaaaddd (back)
    */

    // Respawn old particles
    for (size_t particleIdx = 0; particleIdx < m_ParticleStorage.positions.size(); particleIdx++) {
        float newParticleAge = ages[particleIdx] - m_ParticleDuration;
        if (newParticleAge < 0.0f) {
            continue;
        }

        createParticle({&positions[particleIdx], &velocities[particleIdx], &newParticleAge});
        ages[particleIdx] = newParticleAge;
    }
}

void ParticleEmitter::render() const
{

}

void ParticleEmitter::spawnParticles()
{
    size_t particleCount = m_ParticleStorage.positions.size();
    size_t newParticleCount = m_EmitterAge * m_ParticlesPerSecond;
    size_t particlesToSpawn = newParticleCount - particleCount;

    if (particlesToSpawn == 0) {
        return;
    }

    for (size_t i = 0; i < particlesToSpawn; i++) {
        // Exact time when the particle should have spawned, relative to the creation of the emitter
        float spawnTime = (particleCount + 1 + i) * m_ParticlesPerSecond;
        float particleAge = m_EmitterAge - spawnTime;

        glm::vec3 position, velocity;
        Particle newParticle = {&position, &velocity, &particleAge};
        createParticle(newParticle);

        m_ParticleStorage.positions.push_back(*newParticle.position);
        m_ParticleStorage.velocities.push_back(*newParticle.velocity);
        m_ParticleStorage.ages.push_back(particleAge);
    }
}

void ParticleEmitter::createParticle(const Particle& particle)
{
    glm::vec3 particleDirection = m_Direction; // TODO: Randomly generate!
    float particleAge = (*particle.age);

    /*
        a = (0, -g, 0)
        v = (0, -gt, 0) + V0
        p = (0, -(gt^2)/2, 0) + V0*t + P0
    */

    float gt = -9.82f * particleAge;
    glm::vec3 V0 = particleDirection * m_InitialSpeed;
    *particle.velocity = glm::vec3(0.0f, gt, 0.0f) + V0;
    *particle.position = glm::vec3(0.0f, gt * particleAge / 2.0f, 0.0f) + V0 * particleAge + m_Position;
}
