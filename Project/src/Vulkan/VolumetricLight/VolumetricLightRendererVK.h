#pragma once

#include "Common/IRenderer.h"
#include "Vulkan/ProfilerVK.h"

class CommandBufferVK;
class CommandPoolVK;
class DescriptorPoolVK;
class DescriptorSetLayoutVK;
class FrameBufferVK;
class GraphicsContextVK;
class ImageVK;
class ImageViewVK;
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
    VolumetricLightRendererVK(GraphicsContextVK* pGraphicsContext, RenderingHandlerVK* pRenderingHandler, LightSetup* pLightSetup);
    ~VolumetricLightRendererVK();

    virtual bool init() override;

    virtual void beginFrame(IScene* pScene) override;
    virtual void endFrame(IScene* pScene) override;

    // Renders volumetric light to a light buffer
    void renderLightBuffer();
    // Applies the light buffer to the backbuffer
    void applyLightBuffer();
    // void submitPointLight(PointLight* pPointLight);

    virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;

private:
    struct PushConstants {
        glm::mat4 worldMatrix;

        alignas(16) glm::vec3 lightPosition, lightColor;
        float lightRadius, lightScatterAmount;

        float particleG;

        uint32_t raymarchSteps;
    };

    bool createCommandPoolAndBuffers();
    bool createRenderPass();
    bool createFrameBuffer();
	bool createPipelineLayout();
	bool createPipeline();
	bool createSphereMesh();
	void createProfiler();

    //bool bindDescriptorSet(PointLight* pPointLight);
    //bool createRenderResources(PointLight* pPointLight);

private:
    LightSetup* m_pLightSetup;
    IMesh* m_pSphereMesh;

    RenderPassVK* m_pLightBufferPass;

    FrameBufferVK* m_pLightFrameBuffer;
    ImageVK* m_pLightBufferImage;
    ImageViewVK* m_pLightBufferImageView;

    GraphicsContextVK* m_pGraphicsContext;
    RenderingHandlerVK* m_pRenderingHandler;
    ProfilerVK* m_pProfiler;
    Timestamp m_TimestampDraw;

    CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];

	DescriptorPoolVK* m_pDescriptorPool;
    DescriptorSetLayoutVK* m_pDescriptorSetLayout;

    PipelineLayoutVK* m_pPipelineLayout;
	PipelineVK* m_pPipeline;

    VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

    SamplerVK* m_pSampler;
};
