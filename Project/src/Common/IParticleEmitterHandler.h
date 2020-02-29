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

class IParticleEmitterHandler
{
public:
    IParticleEmitterHandler();
    virtual ~IParticleEmitterHandler();

    virtual void update(float dt) = 0;
    virtual void updateRenderingBuffers(IRenderingHandler* pRenderingHandler) = 0;
    virtual void drawProfileUI() = 0;

    void initialize(IGraphicsContext* pGraphicsContext, const Camera* pCamera);
    // Initializes resources for GPU-side computing of particles
    virtual bool initializeGPUCompute() = 0;

    ParticleEmitter* createEmitter(const ParticleEmitterInfo& emitterInfo);

    std::vector<ParticleEmitter*>& getParticleEmitters() { return m_ParticleEmitters; }
    ParticleEmitter* getParticleEmitter(size_t idx) { return m_ParticleEmitters[idx]; }

    bool gpuComputed() const { return m_GPUComputed; }
    virtual void toggleComputationDevice() = 0;

protected:
    IGraphicsContext* m_pGraphicsContext;
    const Camera* m_pCamera;

    std::vector<ParticleEmitter*> m_ParticleEmitters;

    // Whether to use the GPU or the CPU for updating particle data
    bool m_GPUComputed;

private:
    virtual void initializeEmitter(ParticleEmitter* pEmitter) = 0;
};
