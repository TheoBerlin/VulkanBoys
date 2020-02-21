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
    // Perform particle logic updates on the GPU
    virtual void updateBuffers(IRenderingHandler* pRenderingHandler) = 0;

    void initialize(IGraphicsContext* pGraphicsContext, const Camera* pCamera);
    // Initializes resources for GPU-side computing of particles
    virtual bool initializeGPUCompute() = 0;

    ParticleEmitter* createEmitter(const ParticleEmitterInfo& emitterInfo);

    std::vector<ParticleEmitter*>& getParticleEmitters() { return m_ParticleEmitters; }

    bool gpuComputed() const { return m_GPUComputed; }
    virtual void toggleComputationDevice() = 0;

protected:
    IGraphicsContext* m_pGraphicsContext;
    const Camera* m_pCamera;

    std::vector<ParticleEmitter*> m_ParticleEmitters;

    // Whether to use the GPU or the CPU for updating particle data
    bool m_GPUComputed;

private:
    virtual void updateGPU(float dt) = 0;
};
