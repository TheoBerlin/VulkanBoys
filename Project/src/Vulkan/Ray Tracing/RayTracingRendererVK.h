#pragma once

#include "Common/IRenderer.h"
#include "Vulkan/ProfilerVK.h"
#include "Vulkan/VulkanCommon.h"

class GraphicsContextVK;
class RenderingHandlerVK;
class SceneVK;
class RayTracingPipelineVK;
class ShaderBindingTableVK;
class DescriptorSetLayoutVK;
class PipelineLayoutVK;
class DescriptorPoolVK;
class DescriptorSetVK;
class CommandPoolVK;
class CommandBufferVK;
class ImageVK;
class ImageViewVK;
class ShaderVK;
class SamplerVK;
class Texture2DVK;
class BufferVK;
class Material;
class MeshVK;

class RayTracingRendererVK : public IRenderer
{
public:
	RayTracingRendererVK(GraphicsContextVK* pContext, RenderingHandlerVK* pRenderingHandler);
	~RayTracingRendererVK();

	virtual bool init() override;

	virtual void beginFrame(IScene* pScene) override;
	virtual void endFrame(IScene* pScene) override;

	virtual void render(IScene* pScene, GBufferVK* pGBuffer);

	virtual void setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY) override;

	void onWindowResize(uint32_t width, uint32_t height);

	void setRayTracingResult(ImageViewVK* pRayTracingResultImageView);

	CommandBufferVK* getComputeCommandBuffer() const;
	ProfilerVK* getProfiler() { return m_pProfiler; }

private:
	bool createCommandPoolAndBuffers();
	bool createPipelineLayouts();
	bool createPipeline();
	bool createUniformBuffers();

	void createProfiler();

private:
	GraphicsContextVK* m_pContext;
	RenderingHandlerVK* m_pRenderingHandler;
	ProfilerVK* m_pProfiler;
	Timestamp m_TimestampTraceRays;

	CommandPoolVK* m_ppComputeCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppComputeCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	RayTracingPipelineVK* m_pRayTracingPipeline;

	PipelineLayoutVK* m_pRayTracingPipelineLayout;

	DescriptorSetVK* m_pRayTracingDescriptorSet;
	DescriptorPoolVK* m_pRayTracingDescriptorPool;
	DescriptorSetLayoutVK* m_pRayTracingDescriptorSetLayout;

	//Temp?
	BufferVK* m_pRayTracingUniformBuffer;

	ShaderVK* m_pRaygenShader;
	ShaderVK* m_pClosestHitShader;
	ShaderVK* m_pClosestHitShadowShader;
	ShaderVK* m_pMissShader;
	ShaderVK* m_pMissShadowShader;

	SamplerVK* m_pSampler;

	bool m_TempSubmitLimit;
};