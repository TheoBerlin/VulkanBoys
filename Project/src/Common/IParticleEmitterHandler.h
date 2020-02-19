#pragma once

#include "Common/ITexture2D.h"
#include "Core/ParticleEmitter.h"

class Camera;
class IGraphicsContext;
class IPipeline;
class IRenderer;
class ITexture2D;
class ParticleEmitter;
class RenderingHandler;
class Texture2DVK;
struct ParticleStorage;

class IParticleEmitterHandler
{
public:
    IParticleEmitterHandler();
    ~IParticleEmitterHandler();

    void update(float dt);
    virtual void updateBuffers(IRenderingHandler* pRenderingHandler) = 0;

    void initialize(IGraphicsContext* pGraphicsContext, const Camera* pCamera);

    ParticleEmitter* createEmitter(const ParticleEmitterInfo& emitterInfo);

    std::vector<ParticleEmitter*>& getParticleEmitters() { return m_ParticleEmitters; }

protected:
    IGraphicsContext* m_pGraphicsContext;
    const Camera* m_pCamera;

    std::vector<ParticleEmitter*> m_ParticleEmitters;
};
