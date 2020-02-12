#pragma once

#include "Common/IRenderingHandler.hpp"
#include "Core/Camera.h"
#include "Vulkan/VulkanCommon.h"

#include <thread>

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

    void setMeshRenderer(IRenderer* pMeshRenderer);
    void setRaytracer(IRenderer* pRaytracer);
    void setParticleRenderer(IRenderer* pParticleRenderer);

    FrameBufferVK** getBackBuffers() { return m_ppBackbuffers; }
    RenderPassVK* getRenderPass() { return m_pRenderPass; }

    // Used by renderers to execute their secondary command buffers
    CommandBufferVK* getCommandBuffer(uint32_t frameIndex) { return m_ppCommandBuffers[frameIndex]; }

private:
    bool createBackBuffers();
    bool createCommandPoolAndBuffers();
    bool createRenderPass();
    bool createSemaphores();

    void releaseBackBuffers();

private:
    GraphicsContextVK* m_pGraphicsContext;
    IRenderer* m_pMeshRenderer, *m_pRaytracer, *m_pParticleRenderer;

    FrameBufferVK* m_ppBackbuffers[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];

    CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];

    RenderPassVK* m_pRenderPass;
    PipelineVK* m_pPipeline;

    size_t m_CurrentFrame;
};
