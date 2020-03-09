#pragma once

#include "Common/IRenderer.h"
#include "Vulkan/ProfilerVK.h"
#include "Vulkan/VulkanCommon.h"

class CommandBufferVK;
class CommandPoolVK;
class DescriptorPoolVK;
class DescriptorSetLayoutVK;
class DescriptorSetVK;
class GraphicsContextVK;
class MeshVK;
class ParticleEmitterHandlerVK;
class ParticleEmitter;
class PipelineLayoutVK;
class PipelineVK;
class RenderingHandlerVK;
class RenderPassVK;
class SamplerVK;

class ParticleRendererVK : public IRenderer
{
	struct QuadVertex {
		glm::vec2 Position, TxCoord;
	};

public:
    ParticleRendererVK(GraphicsContextVK* pGraphicsContext, RenderingHandlerVK* pRenderingHandler);
    ~ParticleRendererVK();

    virtual bool init() override;

	virtual void beginFrame(const Camera& camera, const LightSetup& lightSetup) override;
	virtual void endFrame() override;

	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;

	void submitParticles(ParticleEmitter* pEmitter);

	FORCEINLINE CommandBufferVK* getCommandBuffer(uint32_t frameindex) const { return m_ppCommandBuffers[frameindex]; }

	ProfilerVK* getProfiler() { return m_pProfiler; }

private:
	bool createCommandPoolAndBuffers();
	bool createPipelineLayout();
	bool createPipeline();
	bool createQuadMesh();
	void createProfiler();

	bool bindDescriptorSet(ParticleEmitter* pEmitter);

private:
	GraphicsContextVK* m_pGraphicsContext;
	RenderingHandlerVK* m_pRenderingHandler;
	ProfilerVK* m_pProfiler;
	Timestamp m_TimestampDraw;

	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];

	DescriptorSetLayoutVK* m_pDescriptorSetLayout;
	DescriptorPoolVK* m_pDescriptorPool;

	PipelineLayoutVK* m_pPipelineLayout;
	PipelineVK* m_pPipeline;

	VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

	MeshVK* m_pQuadMesh;
	SamplerVK* m_pSampler;
};
