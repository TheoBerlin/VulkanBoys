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
class ParticleEmitter;
class PipelineLayoutVK;
class PipelineVK;
class RenderingHandlerVK;
class RenderPassVK;

class ParticleRendererVK : public IRenderer
{
	struct QuadVertex {
		glm::vec2 Position, TxCoord;
	};

public:
    ParticleRendererVK(GraphicsContextVK* pGraphicsContext, RenderingHandlerVK* pRenderingHandler);
    ~ParticleRendererVK();

    bool init();

	void beginFrame(const Camera& camera);
	void endFrame();

	void beginRayTraceFrame(const Camera& camera);
	void endRayTraceFrame();
	void traceRays();

	void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY);

	void submitParticles(const ParticleEmitter* pParticleEmitter);

private:
	bool createCommandPoolAndBuffers();
	bool createPipelineLayout();
	bool createPipeline();
	bool createQuadMesh();

private:
	GraphicsContextVK* m_pGraphicsContext;
	RenderingHandlerVK* m_pRenderingHandler;

	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];

	DescriptorSetLayoutVK* m_pDescriptorSetLayout;
	DescriptorPoolVK* m_pDescriptorPool;
	DescriptorSetVK* m_pDescriptorSet;

	RenderPassVK* m_pRenderPass;
	PipelineLayoutVK* m_pPipelineLayout;
	PipelineVK* m_pPipeline;

	MeshVK* m_pQuadMesh;
};
