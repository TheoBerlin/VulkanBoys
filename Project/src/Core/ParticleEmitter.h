#pragma once

#include "glm/glm.hpp"

#include "Common/IGraphicsContext.h"

#include <random>

class Camera;
class IBuffer;
class IGraphicsContext;
class IMesh;
class ITexture2D;

struct ParticleEmitterInfo {
    glm::vec3 position, direction;
    glm::vec2 particleSize;
    float particleDuration, initialSpeed, particlesPerSecond;
    // The angle by which spawned particles' directions can diverge from the emitter's direction, [0,pi], where pi means particles can be fired in any direction
    float spread;
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
        std::vector<float> ages, cameraDistances;
    };

public:
    ParticleEmitter(const ParticleEmitterInfo& emitterInfo);
    ~ParticleEmitter();

    bool initialize(IGraphicsContext* pGraphicsContext, const Camera* pCamera);

    void update(float dt);

    const ParticleStorage& getParticleStorage() const { return m_ParticleStorage; }
    glm::vec2 getParticleSize() const { return m_ParticleSize; }
    uint32_t getParticleCount() const { return (uint32_t)m_ParticleStorage.positions.size(); }

    IBuffer* getParticleBuffer() { return m_pParticleBuffer; }
    IBuffer* getEmitterBuffer() { return m_pEmitterBuffer; }
    ITexture2D* getParticleTexture() { return m_pTexture; }

private:
    bool createBuffers(IGraphicsContext* pGraphicsContext);

    // Spawns particles before the emitter has created its maximum amount of particles
    void spawnNewParticles();
    void moveParticles(float dt);
    void calculateCameraDistances();
    // Sort particles by their distance to the camera
    void sortParticles();
    void respawnOldParticles();
    void createParticle(size_t particleIdx, float particleAge);

private:
    glm::vec3 m_Position, m_Direction;
    glm::vec2 m_ParticleSize;
    float m_ParticleDuration, m_InitialSpeed, m_ParticlesPerSecond, m_Spread;

    std::mt19937 randEngine;
    std::uniform_real_distribution<float> zRandomizer;
    std::uniform_real_distribution<float> phiRandomizer;

    ParticleStorage m_ParticleStorage;
    ITexture2D* m_pTexture;

    // The amount of time since the emitter started emitting particles. Used for spawning particles.
    float m_EmitterAge;

    // Contains particle positions
    IBuffer* m_pParticleBuffer;
    // Contains particle size
    IBuffer* m_pEmitterBuffer;

    const Camera* m_pCamera;
};
