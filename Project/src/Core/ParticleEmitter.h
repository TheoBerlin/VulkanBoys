#pragma once

#include "glm/glm.hpp"

#include "Common/IGraphicsContext.h"

class IBuffer;
class IGraphicsContext;
class IMesh;
class ITexture2D;

struct ParticleEmitterInfo {
    glm::vec3 position, direction;
    glm::vec2 particleSize;
    float particleDuration, initialSpeed, particlesPerSecond;
    ITexture2D* pTexture;
};

struct ParticleBuffer {
    const glm::vec4* positions;
};

struct EmitterBuffer {
    glm::vec2 particleSize;
};

class ParticleEmitter
{
    struct Particle {
        glm::vec4* position, *velocity;
        float* age;
    };

    struct ParticleStorage {
        std::vector<glm::vec4> positions, velocities;
        std::vector<float> ages;
    };

public:
    ParticleEmitter(const ParticleEmitterInfo& emitterInfo);
    ~ParticleEmitter();

    bool initialize(IGraphicsContext* pGraphicsContext);

    void update(float dt);

    const ParticleStorage& getParticleStorage() const { return m_ParticleStorage; }
    glm::vec2 getParticleSize() const { return m_ParticleSize; }
    uint32_t getParticleCount() const { return (uint32_t)m_ParticleStorage.positions.size(); }

    IBuffer* getParticleBuffer() { return m_pParticleBuffer; }
    IBuffer* getEmitterBuffer() { return m_pEmitterBuffer; }

private:
    bool createBuffers(IGraphicsContext* pGraphicsContext);

    // Spawns particles before the emitter has created its maximum amount of particles
    void spawnNewParticles();
    void respawnOldParticles();
    void createParticle(size_t particleIdx, float particleAge);

private:
    glm::vec4 m_Position, m_Direction;
    glm::vec2 m_ParticleSize;
    float m_ParticleDuration, m_InitialSpeed, m_ParticlesPerSecond;

    ParticleStorage m_ParticleStorage;
    ITexture2D* m_pTexture;

    // The amount of time since the emitter started emitting particles. Used for spawning particles.
    float m_EmitterAge;

    // Contains particle positions
    IBuffer* m_pParticleBuffer;
    // Contains particle size
    IBuffer* m_pEmitterBuffer;
};
