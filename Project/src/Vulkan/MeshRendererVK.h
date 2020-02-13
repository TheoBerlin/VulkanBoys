#pragma once
#include "Common/IRenderer.h"
#include "VulkanCommon.h"

class BufferVK;
class PipelineVK;
class RenderingHandlerVK;
class RenderPassVK;
class FrameBufferVK;
class CommandPoolVK;
class CommandBufferVK;
class DescriptorSetVK;
class DescriptorPoolVK;
class PipelineLayoutVK;
class GraphicsContextVK;
class DescriptorSetLayoutVK;

class MeshRendererVK : public IRenderer
{
public:
	MeshRendererVK(GraphicsContextVK* pContext, RenderingHandlerVK* pRenderingHandler);
	~MeshRendererVK();

	virtual bool init() override;

	virtual void beginFrame(const Camera& camera) override;
	virtual void endFrame() override;
	virtual void execute() override;

	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;

	virtual void submitMesh(IMesh* pMesh, const glm::vec4& color, const glm::mat4& transform) override;

	virtual void drawImgui(IImgui* pImgui) override;

private:
	bool createSemaphores();
	bool createCommandPoolAndBuffers();
	bool createPipelines();
	bool createPipelineLayouts();

private:
	GraphicsContextVK* m_pContext;
	RenderingHandlerVK* m_pRenderingHandler;
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	RenderPassVK* m_pRenderPass;
	FrameBufferVK* m_ppBackbuffers[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];

	// TEMPORARY MOVE TO MATERIAL or SOMETHING
	PipelineVK* m_pPipeline;
	PipelineLayoutVK* m_pPipelineLayout;
	DescriptorSetVK* m_pDescriptorSet;
	DescriptorPoolVK* m_pDescriptorPool;
	DescriptorSetLayoutVK* m_pDescriptorSetLayout;

	VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

	uint64_t m_CurrentFrame;
	uint32_t m_BackBufferIndex;
};
