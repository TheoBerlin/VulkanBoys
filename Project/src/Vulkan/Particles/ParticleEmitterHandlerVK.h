#pragma once

#include "Common/IParticleEmitterHandler.h"
#include "Common/ITexture2D.h"
#include "Core/ParticleEmitter.h"
#include "Vulkan/ProfilerVK.h"
#include "Vulkan/VulkanCommon.h"

class BufferVK;
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

    virtual void update(float dt) override;
    virtual void updateRenderingBuffers(IRenderingHandler* pRenderingHandler) override;
    virtual void drawProfilerUI() override;

    virtual bool initializeGPUCompute() override;

    virtual void toggleComputationDevice() override;

    void releaseFromGraphics(BufferVK* pBuffer, CommandBufferVK* pCommandBuffer);
    void releaseFromCompute(BufferVK* pBuffer, CommandBufferVK* pCommandBuffer);
    void acquireForGraphics(BufferVK* pBuffer, CommandBufferVK* pCommandBuffer);
    void acquireForCompute(BufferVK* pBuffer, CommandBufferVK* pCommandBuffer);

private:
    // Initializes an emitter and prepares its buffers for computing or rendering
    virtual void initializeEmitter(ParticleEmitter* pEmitter) override;

    void updateGPU(float dt);

    void beginUpdateFrame();
    void endUpdateFrame();

    bool createCommandPoolAndBuffers();
    bool createPipelineLayout();
    bool createPipeline();
    void createProfiler();

private:
    CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];
    CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];

    // Used for creating temporary graphics command buffers for initializing emitter buffers
    CommandPoolVK* m_pCommandPoolGraphics;

    DescriptorPoolVK* m_pDescriptorPool;
    DescriptorSetLayoutVK* m_pDescriptorSetLayout;
    DescriptorSetVK* m_ppDescriptorSets[MAX_FRAMES_IN_FLIGHT];

    PipelineLayoutVK* m_pPipelineLayout;
    PipelineVK* m_pPipeline;

    // Work items per work group launched in a compute shader dispatch
    uint32_t m_WorkGroupSize;

    uint32_t m_CurrentFrame;

    ProfilerVK* m_pProfiler;
    Timestamp m_TimestampDispatch;
};
