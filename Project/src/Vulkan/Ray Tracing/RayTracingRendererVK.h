#pragma once

#include "Common/IRenderer.h"
#include "Vulkan/ProfilerVK.h"
#include "Vulkan/VulkanCommon.h"

class DescriptorSetLayoutVK;
class RayTracingPipelineVK;
class ShaderBindingTableVK;
class RenderingHandlerVK;
class GraphicsContextVK;
class PipelineLayoutVK;
class DescriptorPoolVK;
class DescriptorSetVK;
class CommandBufferVK;
class CommandPoolVK;
class TextureCubeVK;
class ImageViewVK;
class Texture2DVK;
class SamplerVK;
class ShaderVK;
class BufferVK;
class Material;
class SceneVK;
class ImageVK;
class MeshVK;

constexpr uint32_t RT_RESULT_IMAGE_BINDING = 0;
constexpr uint32_t RT_CAMERA_BUFFER_BINDING = 1;
constexpr uint32_t RT_TLAS_BINDING = 2;
constexpr uint32_t RT_GBUFFER_ALBEDO_BINDING = 3;
constexpr uint32_t RT_GBUFFER_NORMAL_BINDING = 4;
constexpr uint32_t RT_GBUFFER_DEPTH_BINDING = 5;
constexpr uint32_t RT_COMBINED_VERTEX_BINDING = 6;
constexpr uint32_t RT_COMBINED_INDEX_BINDING = 7;
constexpr uint32_t RT_MESH_INDEX_BINDING = 8;
constexpr uint32_t RT_COMBINED_ALBEDO_BINDING = 9;
constexpr uint32_t RT_COMBINED_NORMAL_BINDING = 10;
constexpr uint32_t RT_COMBINED_AO_BINDING = 11;
constexpr uint32_t RT_COMBINED_METALLIC_BINDING = 12;
constexpr uint32_t RT_COMBINED_ROUGHNESS_BINDING = 13;
constexpr uint32_t RT_COMBINED_MATERIAL_PARAMETERS_BINDING = 14;
constexpr uint32_t RT_SKYBOX_BINDING = 15;
constexpr uint32_t RT_LIGHT_BUFFER_BINDING = 16;

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

	void setRayTracingResult(ImageViewVK* pRayTracingResultImageView, uint32_t width, uint32_t height);
	void setSkybox(TextureCubeVK* pSkybox);


	CommandBufferVK* getComputeCommandBuffer() const;
	ProfilerVK* getProfiler() { return m_pProfiler; }

private:
	bool createCommandPoolAndBuffers();
	bool createPipelineLayouts();
	bool createPipeline();
	bool createUniformBuffers();

	void createProfiler();

	void updateBuffers(SceneVK* pScene, CommandBufferVK* pCommandBuffer);

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

	TextureCubeVK* m_pSkybox;

	//Temp?
	BufferVK* m_pCameraBuffer;
	BufferVK* m_pLightsBuffer;

	ShaderVK* m_pRaygenShader;
	ShaderVK* m_pClosestHitShader;
	ShaderVK* m_pClosestHitShadowShader;
	ShaderVK* m_pMissShader;
	ShaderVK* m_pMissShadowShader;

	SamplerVK* m_pSampler;

	uint32_t m_RaysWidth;
	uint32_t m_RaysHeight;

	bool m_TempSubmitLimit;
};