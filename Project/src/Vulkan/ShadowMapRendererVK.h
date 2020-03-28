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

	void updateBuffers(SceneVK* pScene);

	const VkViewport& getViewport() const { return m_Viewport; }
	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;
	void onWindowResize(uint32_t width, uint32_t height);

	void submitMesh(const MeshVK* pMesh, const Material* pMaterial, uint32_t transformIndex);

	FORCEINLINE CommandBufferVK*	getCommandBuffer(uint32_t frameindex) const { return m_ppCommandBuffers[frameindex]; }
	FORCEINLINE ProfilerVK*			getProfiler()								{ return m_pProfiler; }

private:
	bool createCommandPoolAndBuffers();
	bool createPipelineLayout();
	bool createPipeline();
	bool createSampler();
	void createProfiler();

	bool createShadowMapResources(DirectionalLight* directionalLight);

private:
	GraphicsContextVK* m_pGraphicsContext;
	RenderingHandlerVK* m_pRenderingHandler;
	ProfilerVK* m_pProfiler;

	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];

	DescriptorSetLayoutVK* m_pDescriptorSetLayout;
	DescriptorPoolVK* m_pDescriptorPool;

	PipelineLayoutVK* m_pPipelineLayout;
	PipelineVK* m_pPipeline;

	VkViewport m_Viewport;
	VkRect2D m_ScissorRect;

	// Updated each frame
	SceneVK* m_pScene;

	SamplerVK* m_pShadowMapSampler;
};
