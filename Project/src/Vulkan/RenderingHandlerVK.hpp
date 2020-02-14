#pragma once

#include "Common/IRenderingHandler.hpp"
#include "Core/Camera.h"
#include "Vulkan/VulkanCommon.h"

#include <thread>

class BufferVK;
class CommandBufferVK;
class CommandPoolVK;
class FrameBufferVK;
class GraphicsContextVK;
class IGraphicsContext;
class IRenderer;
class PipelineVK;
class RenderPassVK;

class RenderingHandlerVK : public IRenderingHandler
{
public:
    RenderingHandlerVK(GraphicsContextVK* pGraphicsContext);
    ~RenderingHandlerVK();

    bool initialize();

    void onWindowResize(uint32_t width, uint32_t height);

    void beginFrame(const Camera& camera);
    void endFrame();
    void swapBuffers();

    void drawImgui(IImgui* pImgui);

    void setClearColor(float r, float g, float b);
    void setClearColor(const glm::vec3& color);
    void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY);

    void submitMesh(IMesh* pMesh, const glm::vec4& color, const glm::mat4& transform);

    FrameBufferVK** getBackBuffers() { return m_ppBackbuffers; }
    RenderPassVK* getRenderPass() { return m_pRenderPass; }
    BufferVK* getCameraBuffer() { return m_pCameraBuffer; }

    // Used by renderers to execute their secondary command buffers
    CommandBufferVK* getCommandBuffer(uint32_t frameIndex) { return m_ppCommandBuffers[frameIndex]; }

private:
    bool createBackBuffers();
    bool createCommandPoolAndBuffers();
    bool createRenderPass();
    bool createSemaphores();
    bool createBuffers();

    void releaseBackBuffers();

    void startRenderPass();

private:
    GraphicsContextVK* m_pGraphicsContext;

    FrameBufferVK* m_ppBackbuffers[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];

    CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];

    CommandPoolVK* m_ppCommandPoolsSecondary[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppCommandBuffersSecondary[MAX_FRAMES_IN_FLIGHT];

    RenderPassVK* m_pRenderPass;
    PipelineVK* m_pPipeline;

    uint32_t m_CurrentFrame, m_BackBufferIndex;

    VkClearValue m_ClearColor;
	VkClearValue m_ClearDepth;

    VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

    BufferVK* m_pCameraBuffer;

    bool m_EnableRayTracing;
};
