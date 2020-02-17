#pragma once

#include "glm/glm.hpp"

#include "Common/IGraphicsContext.h"

/*struct ParticleEmitterCreateInfo {
    glm::vec3 position, direction;
    // For how long each particle exists
    float particleDuration;
    float initialVelocity;
};*/

class IGraphicsContext;
class IMesh;
class ITexture2D;

class ParticleEmitter
{
    struct Particle {
        glm::vec3* position, *velocity;
        float* age;
    };

    struct ParticleStorage {
        std::vector<glm::vec3> positions, velocities;
        std::vector<float> ages;
    };

public:
    ParticleEmitter(glm::vec3 position, glm::vec3 direction, float particleDuration, float initialSpeed, float particlesPerSecond);
    ~ParticleEmitter();

    void initialize(IGraphicsContext* pGraphicsContext);

    void update(float dt);

    const ParticleStorage& getParticleStorage() const { return m_ParticleStorage; }

private:
    // Spawns particles before the emitter has created its maximum amount of particles
    void spawnParticles();
    void createParticle(const Particle& particle);

private:
    glm::vec3 m_Position, m_Direction;
    float m_ParticleDuration, m_InitialSpeed, m_ParticlesPerSecond;

    ParticleStorage m_ParticleStorage;
    ITexture2D* m_pTexture;

    // The amount of time since the emitter started emitting particles. Used for spawning particles.
    float m_EmitterAge;
};
