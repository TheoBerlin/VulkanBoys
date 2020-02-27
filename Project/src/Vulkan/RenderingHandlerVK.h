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
class MeshRendererVK;
class ParticleRendererVK;
class PipelineVK;
class RayTracerVK;
class RenderPassVK;

class RenderingHandlerVK : public IRenderingHandler
{
public:
    RenderingHandlerVK(GraphicsContextVK* pGraphicsContext);
    ~RenderingHandlerVK();

    bool initialize();

    void setMeshRenderer(IRenderer* pMeshRenderer) { m_pMeshRenderer = reinterpret_cast<MeshRendererVK*>(pMeshRenderer); }
    void setRayTracer(IRenderer* pRayTracer) { m_pRayTracer = pRayTracer; }
    void setParticleRenderer(IRenderer* pParticleRenderer) { m_pParticleRenderer = reinterpret_cast<ParticleRendererVK*>(pParticleRenderer); }

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
    BufferVK* getCameraMatricesBuffer() { return m_pCameraMatricesBuffer; }
    BufferVK* getCameraDirectionsBuffer() { return m_pCameraDirectionsBuffer; }

    // Used by renderers to execute their secondary command buffers
    CommandBufferVK* getCurrentGraphicsCommandBuffer() { return m_ppGraphicsCommandBuffers[m_CurrentFrame]; }
	CommandBufferVK* getCurrentComputeCommandBuffer() { return m_ppComputeCommandBuffers[m_CurrentFrame]; }

    uint32_t getCurrentFrameIndex() const { return m_CurrentFrame; }
    FrameBufferVK* getCurrentBackBuffer() const { return m_ppBackbuffers[m_BackBufferIndex]; }

private:
    bool createBackBuffers();
    bool createCommandPoolAndBuffers();
    bool createRenderPass();
    bool createSemaphores();
    bool createBuffers();

    void releaseBackBuffers();

    void updateBuffers(const Camera& camera);

    void startRenderPass();
    void submitParticles();

private:
    GraphicsContextVK* m_pGraphicsContext;

    MeshRendererVK* m_pMeshRenderer;
    ParticleRendererVK* m_pParticleRenderer;
    // TODO: Implement renderer class for ray tracing
    IRenderer* m_pRayTracer;

    FrameBufferVK* m_ppBackbuffers[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore m_RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];

    CommandPoolVK* m_ppGraphicsCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppGraphicsCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	CommandPoolVK* m_ppComputeCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppComputeCommandBuffers[MAX_FRAMES_IN_FLIGHT];

    CommandPoolVK* m_ppCommandPoolsSecondary[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppCommandBuffersSecondary[MAX_FRAMES_IN_FLIGHT];


    RenderPassVK* m_pRenderPass;
    PipelineVK* m_pPipeline;

    uint32_t m_CurrentFrame, m_BackBufferIndex;

    VkClearValue m_ClearColor;
	VkClearValue m_ClearDepth;

    VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

    BufferVK* m_pCameraMatricesBuffer, *m_pCameraDirectionsBuffer;

    bool m_EnableRayTracing;
};
