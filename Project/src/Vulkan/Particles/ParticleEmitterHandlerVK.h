#pragma once

#include "Common/IParticleEmitterHandler.h"
#include "Common/ITexture2D.h"
#include "Core/ParticleEmitter.h"
#include "Vulkan/VulkanCommon.h"

class CommandBufferVK;
class CommandPoolVK;
class DescriptorPoolVK;
class DescriptorSetLayoutVK;
class DescriptorSetVK;
class IPipeline;
class IGraphicsContext;
class IRenderer;
class ITexture2D;
class ParticleEmitter;
class PipelineLayoutVK;
class PipelineVK;
class SamplerVK;
class Texture2DVK;

struct ParticleStorage;

class ParticleEmitterHandlerVK : public IParticleEmitterHandler
{
public:
    ParticleEmitterHandlerVK();
    ~ParticleEmitterHandlerVK();

    virtual void updateBuffers(IRenderingHandler* pRenderingHandler) override;
    virtual bool initializeGPUCompute() override;

    virtual void toggleComputationDevice() override;

private:
    void updateGPU(float dt);

    bool createCommandPoolAndBuffers();
    bool createPipelineLayout();
    bool createPipeline();

    void writeBufferDescriptors();

private:
    CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];
    CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];

    DescriptorPoolVK* m_pDescriptorPool;
    DescriptorSetLayoutVK* m_pDescriptorSetLayout;
    DescriptorSetVK* m_ppDescriptorSets[MAX_FRAMES_IN_FLIGHT];

    PipelineLayoutVK* m_pPipelineLayout;
    PipelineVK* m_pPipeline;

    SamplerVK* m_pSampler;
};
