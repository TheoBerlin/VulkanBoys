#pragma once
#include "Common/IRenderer.h"
#include "VulkanCommon.h"

class ContextVK;
class CommandPoolVK;
class CommandBufferVK;
class PipelineStateVK;
class RenderPassVK;
class FramebufferVK;

class RendererVK : public IRenderer
{
public:
	RendererVK(ContextVK* pContext);
	~RendererVK();

	virtual bool init() override;

	virtual void beginFrame() override;
	virtual void endFrame() override;

	virtual void setClearColor(float r, float g, float b) override;
	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;

	//Temporary function
	virtual void drawTriangle() override;

private:
	ContextVK* m_pContext;
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];
	RenderPassVK* m_pRenderPass;
	FramebufferVK* m_ppBackbuffers[MAX_FRAMES_IN_FLIGHT];

	//TEMPORARY MOVE TO MATERIAL or SOMETHING
	PipelineStateVK* m_pPipelineState;

	VkClearValue m_ClearColor;
	VkViewport m_Viewport;
	VkRect2D m_ScissorRect;
};

