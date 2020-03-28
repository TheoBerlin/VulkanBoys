#pragma once

#include "Common/IRenderer.h"
#include "Core/VolumetricPointLight.h"
#include "Vulkan/ProfilerVK.h"

#include "imgui/imgui.h"

class CommandBufferVK;
class CommandPoolVK;
class DescriptorPoolVK;
class DescriptorSetLayoutVK;
class DescriptorSetVK;
class FrameBufferVK;
class GraphicsContextVK;
class ImageVK;
class ImageViewVK;
class ImguiVK;
class LightSetup;
class PipelineVK;
class PipelineLayoutVK;
class PointLight;
class RenderingHandlerVK;
class RenderPassVK;
class SamplerVK;

class VolumetricLightRendererVK : public IRenderer
{
public:
    VolumetricLightRendererVK(GraphicsContextVK* pGraphicsContext, RenderingHandlerVK* pRenderingHandler, LightSetup* pLightSetup, ImguiVK* pImguiRenderer);
    ~VolumetricLightRendererVK();

    virtual bool init() override;

    void updateBuffers();

    virtual void beginFrame(IScene* pScene) override;
    virtual void endFrame(IScene* pScene) override;

    // Renders volumetric light to a light buffer
    void renderLightBuffer();
    // Applies the light buffer to the backbuffer
    void applyLightBuffer();

    virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;
    void onWindowResize(uint32_t width, uint32_t height);

    virtual void renderUI() override;

    FrameBufferVK* getLightFrameBuffer() { return m_pLightFrameBuffer; }
    VkClearValue getLightBufferClearColor() { return m_LightBufferClearColor; }
    RenderPassVK* getLightBufferPass() { return m_pLightBufferPass; }
    const VkViewport& getViewport() const { return m_Viewport; }

    CommandBufferVK* getCommandBuffer(uint32_t frameIndex) { return m_ppCommandBuffers[frameIndex]; }
    ProfilerVK* getProfiler() { return m_pProfiler; }

private:
    struct PushConstants {
        glm::vec2 viewportExtent;
        uint32_t raymarchSteps;
    };

    void renderPointLights();
    void renderDirectionalLight();

    bool createCommandPoolAndBuffers();
    bool createRenderPass();
    bool createFrameBuffer();
	bool createPipelineLayout();
	bool createPipelines();
	bool createSphereMesh();
	void createProfiler();

    bool createRenderResources(VolumetricPointLight& pointLight);

private:
    LightSetup* m_pLightSetup;
    ImguiVK* m_pImguiRenderer;
    IMesh* m_pSphereMesh;

    RenderPassVK* m_pLightBufferPass;

    FrameBufferVK* m_pLightFrameBuffer;
    ImageVK* m_pLightBufferImage;
    ImageViewVK* m_pLightBufferImageView;
    VkClearValue m_LightBufferClearColor;
    ImTextureID m_LightBufferImID;

    GraphicsContextVK* m_pGraphicsContext;
    RenderingHandlerVK* m_pRenderingHandler;
    ProfilerVK* m_pProfiler;
    Timestamp m_TimestampDraw;

    CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];

	DescriptorPoolVK* m_pDescriptorPool;
    DescriptorSetLayoutVK* m_pDescriptorSetLayoutCommon;
    DescriptorSetLayoutVK* m_pDescriptorSetLayoutPerLight;
    DescriptorSetVK* m_pDescriptorSetCommon;

    PipelineLayoutVK* m_pPipelineLayout;

	PipelineVK* m_pPipelinePointLight;
	PipelineVK* m_pPipelineDirectionalLight;

    VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

    SamplerVK* m_pSampler;

    // ImGui resources
    int m_CurrentIndex;
};
