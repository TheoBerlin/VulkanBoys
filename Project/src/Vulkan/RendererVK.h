#pragma once
#include "Common/IRenderer.h"
#include "VulkanCommon.h"

class GraphicsContextVK;
class PipelineVK;
class RenderPassVK;
class FrameBufferVK;
class CommandPoolVK;
class CommandBufferVK;
class PipelineLayoutVK;

class RendererVK : public IRenderer
{
public:
	RendererVK(GraphicsContextVK* pContext);
	~RendererVK();

	virtual bool init() override;

	virtual void onWindowResize(uint32_t width, uint32_t height) override;

	virtual void beginFrame(const Camera& camera) override;
	virtual void endFrame() override;

	virtual void setClearColor(float r, float g, float b) override;
	virtual void setClearColor(const glm::vec3& color) override;
	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;

	virtual void swapBuffers() override;

	virtual void drawImgui(IImgui* pImgui) override;

	//Temporary function
	virtual void drawTriangle(const glm::vec4& color) override;

private:
	void createFramebuffers();
	void releaseFramebuffers();

private:
	GraphicsContextVK* m_pContext;
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];
	RenderPassVK* m_pRenderPass;
	FrameBufferVK* m_ppBackbuffers[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];

	//TEMPORARY MOVE TO MATERIAL or SOMETHING
	PipelineVK* m_pPipeline;
	PipelineLayoutVK* m_pPipelineLayout;

	VkClearValue m_ClearColor;
	VkClearValue m_ClearDepth;
	VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

	uint64_t m_CurrentFrame;
	uint32_t m_BackBufferIndex;
};

