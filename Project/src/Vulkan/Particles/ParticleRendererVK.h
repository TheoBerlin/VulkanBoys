#pragma once

#include "Common/IRenderer.h"
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

	virtual void beginFrame(const Camera& camera) override;
	virtual void endFrame() override;

	void submitParticles(ParticleEmitterHandlerVK* pEmitterHandler);
	void submitParticles(ParticleEmitter* pEmitter);

	void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY);

private:
	bool createCommandPoolAndBuffers();
	bool createPipelineLayout();
	bool createPipeline();
	bool createQuadMesh();
	void writeBufferDescriptors();

private:
	GraphicsContextVK* m_pGraphicsContext;
	RenderingHandlerVK* m_pRenderingHandler;

	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];

	DescriptorSetLayoutVK* m_pDescriptorSetLayout;
	DescriptorPoolVK* m_pDescriptorPool;
	DescriptorSetVK* m_ppDescriptorSets[MAX_FRAMES_IN_FLIGHT];

	PipelineLayoutVK* m_pPipelineLayout;
	PipelineVK* m_pPipeline;

	VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

	MeshVK* m_pQuadMesh;
	SamplerVK* m_pSampler;
};
