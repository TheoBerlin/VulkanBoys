#pragma once

#include "Common/ITexture2D.h"

class IPipeline;
class IGraphicsContext;
class ITexture2D;
class ParticleEmitter;

struct ParticleEmitterInfo {
    glm::vec3 position, direction;
    float particleDuration, initialSpeed, particlesPerSecond;
    ITexture2D* pTexture;
};

class IRenderer;
class Texture2DVK;
struct ParticleStorage;

class ParticleEmitterHandler
{
public:
    ParticleEmitterHandler();
    ~ParticleEmitterHandler();

    void initialize(IGraphicsContext* pGraphicsContext);

    ParticleEmitter* createEmitter(const ParticleEmitterInfo& emitterInfo);

    const std::vector<ParticleEmitter*>& getParticleEmitters() const { return m_ParticleEmitters; }

private:
    IGraphicsContext* m_pGraphicsContext;

    std::vector<ParticleEmitter*> m_ParticleEmitters;
};
