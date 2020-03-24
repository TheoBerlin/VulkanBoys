#pragma once
#include "Common/RenderingHandler.hpp"
#include "Core/Camera.h"

#include "Vulkan/VulkanCommon.h"

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
class RayTracingRendererVK;
class RenderPassVK;
class MeshVK;
class SkyboxRendererVK;
class ImguiVK;
class GBufferVK;
class IScene;
class SceneVK;
class ImageVK;
class ImageViewVK;

class RenderingHandlerVK : public RenderingHandler
{
public:
    RenderingHandlerVK(GraphicsContextVK* pGraphicsContext);
    ~RenderingHandlerVK();

    virtual bool initialize() override;

    virtual ITextureCube* generateTextureCube(ITexture2D* pPanorama, ETextureFormat format, uint32_t width, uint32_t miplevels) override;

	virtual void render(IScene* pScene) override;

    virtual void swapBuffers() override;

    virtual void drawProfilerUI() override;

	virtual void setRayTracer(IRenderer* pRayTracer) override				{ m_pRayTracer = reinterpret_cast<RayTracingRendererVK*>(pRayTracer); }
    virtual void setMeshRenderer(IRenderer* pMeshRenderer) override         { m_pMeshRenderer = reinterpret_cast<MeshRendererVK*>(pMeshRenderer); }
    virtual void setParticleRenderer(IRenderer* pParticleRenderer) override { m_pParticleRenderer = reinterpret_cast<ParticleRendererVK*>(pParticleRenderer); }
    virtual void setImguiRenderer(IImgui* pImGui) override                  { m_pImGuiRenderer = reinterpret_cast<ImguiVK*>(pImGui); }

    virtual void setClearColor(float r, float g, float b) override;
    virtual void setClearColor(const glm::vec3& color) override;
    virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;
    virtual void setSkybox(ITextureCube* pSkybox) override;

    virtual void onWindowResize(uint32_t width, uint32_t height) override;
	virtual void onSceneUpdated(IScene* pScene) override;

	virtual void setRayTracingResolutionDenominator(uint32_t denom) override;

    uint32_t                getCurrentFrameIndex() const                { return m_CurrentFrame; }
    FrameBufferVK* const*   getBackBuffers() const                      { return m_ppBackbuffers; }
	RenderPassVK*			getGeometryRenderPass() const				{ return m_pGeometryRenderPass; }
    RenderPassVK*           getBackBufferRenderPass() const             { return m_pBackBufferRenderPass; }
    RenderPassVK*           getParticleRenderPass() const               { return m_pParticleRenderPass; }
    BufferVK*               getCameraBufferCompute() const              { return m_pCameraBufferCompute; }
    BufferVK*               getCameraBufferGraphics() const             { return m_pCameraBufferGraphics; }
    BufferVK*               getLightBufferCompute() const               { return m_pLightBufferCompute; }
    BufferVK*               getLightBufferGraphics() const              { return m_pLightBufferGraphics; }
    FrameBufferVK*          getCurrentBackBuffer() const                { return m_ppBackbuffers[m_BackBufferIndex]; }
    FrameBufferVK*          getCurrentBackBufferWithDepth() const       { return m_ppBackBuffersWithDepth[m_BackBufferIndex]; }
    CommandBufferVK*        getCurrentGraphicsCommandBuffer() const     { return m_ppGraphicsCommandBuffers[m_CurrentFrame]; }
	GBufferVK*				getGBuffer() const							{ return m_pGBuffer; }

private:
    bool createBackBuffers();
    bool createCommandPoolAndBuffers();
    bool createRenderPasses();
    bool createSemaphores();
    bool createBuffers();
	bool createRayTracingRenderImages(uint32_t width, uint32_t height);
	bool createGBuffer();

    void releaseBackBuffers();

    void updateBuffers(SceneVK* pScene, const Camera& camera, const LightSetup& lightSetup);

    void submitParticles();

private:
    CameraBuffer m_CameraBuffer;
    
    GraphicsContextVK* m_pGraphicsContext;

    SkyboxRendererVK*       m_pSkyboxRenderer;
    MeshRendererVK*         m_pMeshRenderer;
    ParticleRendererVK*     m_pParticleRenderer;
    RayTracingRendererVK*   m_pRayTracer;
    ImguiVK*                m_pImGuiRenderer;

    FrameBufferVK*  m_ppBackbuffers[MAX_FRAMES_IN_FLIGHT];
    FrameBufferVK*  m_ppBackBuffersWithDepth[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore     m_pImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore     m_pRenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore     m_ComputeFinishedGraphicsSemaphore;
    VkSemaphore     m_ComputeFinishedTransferSemaphore;
    VkSemaphore     m_GeometryFinishedSemaphore;
    VkSemaphore     m_TransferStartSemaphore;
    VkSemaphore     m_TransferFinishedGraphicsSemaphore;
    VkSemaphore     m_TransferFinishedComputeSemaphore;

    CommandPoolVK*      m_ppGraphicsCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK*    m_ppGraphicsCommandBuffers[MAX_FRAMES_IN_FLIGHT];
    CommandBufferVK*    m_ppGraphicsCommandBuffers2[MAX_FRAMES_IN_FLIGHT];
	CommandPoolVK*      m_ppTransferCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK*    m_ppTransferCommandBuffers[MAX_FRAMES_IN_FLIGHT];
    CommandPoolVK*      m_ppComputeCommandPools[MAX_FRAMES_IN_FLIGHT];
    CommandBufferVK*    m_ppComputeCommandBuffers[MAX_FRAMES_IN_FLIGHT];
    CommandPoolVK*      m_ppCommandPoolsSecondary[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK*    m_ppCommandBuffersSecondary[MAX_FRAMES_IN_FLIGHT];

	RenderPassVK*   m_pGeometryRenderPass;
    RenderPassVK*   m_pBackBufferRenderPass;
    RenderPassVK*   m_pParticleRenderPass;
    RenderPassVK*   m_pUIRenderPass;
    PipelineVK*     m_pPipeline;

    uint32_t m_CurrentFrame;
    uint32_t m_BackBufferIndex;

    VkClearValue m_ClearColor;
	VkClearValue m_ClearDepth;

    VkViewport  m_Viewport;
	VkRect2D    m_ScissorRect;

    BufferVK*   m_pCameraBufferGraphics;
    BufferVK*   m_pCameraBufferCompute;
    BufferVK*   m_pLightBufferGraphics;
    BufferVK*   m_pLightBufferCompute;
    GBufferVK*  m_pGBuffer;

	//Render Results
	uint32_t m_RayTracingResolutionDenominator;

	union
	{
		struct
		{
			ImageVK* m_pRadianceImage;
			ImageVK* m_pGlossyImage;
		};

		ImageVK* m_RayTracingImages[2];
	};

	union
	{
		struct
		{
			ImageViewVK* m_pRadianceImageView;
			ImageViewVK* m_pGlossyImageView;
		};

		ImageVK* m_RayTracingImageViews[2];
	};
};
