#pragma once

#include "ParticleEmitter.hpp"

class IPipeline;
class IGraphicsContext;

class ParticleEmitterHandler
{
public:
    ParticleEmitterHandler();
    ~ParticleEmitterHandler();

    void initialize(IGraphicsContext* pGraphicsContext);
    void render();

    void addEmitter(glm::vec3 position, glm::vec3 direction, float particleDuration, float initialSpeed, float particlesPerSecond);

private:
    std::vector<ParticleEmitter> m_ParticleEmitters;
    IGraphicsContext* m_pGraphicsContext;

};
