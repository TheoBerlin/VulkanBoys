#pragma once

#include "Common/IRenderer.h"
#include "Vulkan/ProfilerVK.h"
#include "Vulkan/VulkanCommon.h"

class CommandBufferVK;
class CommandPoolVK;
class DescriptorPoolVK;
class DescriptorSetLayoutVK;
class DescriptorSetVK;
class DirectionalLight;
class GraphicsContextVK;
class MeshVK;
class PipelineLayoutVK;
class PipelineVK;
class RenderingHandlerVK;
class RenderPassVK;
class SamplerVK;
class SceneVK;

class ShadowMapRendererVK : public IRenderer
{
public:
    ShadowMapRendererVK(GraphicsContextVK* pGraphicsContext, RenderingHandlerVK* pRenderingHandler);
    ~ShadowMapRendererVK();

    virtual bool init() override;

	virtual void beginFrame(IScene* pScene) override;
	virtual void endFrame(IScene* pScene) override;

	virtual void renderUI() override;

	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;

	void submitMesh(const MeshVK* pMesh, const Material* pMaterial, uint32_t transformIndex);

	FORCEINLINE CommandBufferVK*	getCommandBuffer(uint32_t frameindex) const { return m_ppCommandBuffers[frameindex]; }
	FORCEINLINE ProfilerVK*			getProfiler()								{ return m_pProfiler; }

private:
	bool createCommandPoolAndBuffers();
	bool createPipeline();
	void createProfiler();

	bool createShadowMapResources(DirectionalLight* directionalLight);

private:
	GraphicsContextVK* m_pGraphicsContext;
	RenderingHandlerVK* m_pRenderingHandler;
	ProfilerVK* m_pProfiler;

	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];

	RenderPassVK* m_pRenderPass;

	PipelineVK* m_pPipeline;

	VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

	// Updated each frame
	SceneVK* m_pScene;
};
