#pragma once
#include "VulkanCommon.h"

class DeviceVK;
class SamplerVK;
class PipelineVK;
class Texture2DVK;
class RenderPassVK;
class TextureCubeVK;
class CommandPoolVK;
class DescriptorSetVK;
class CommandBufferVK;
class PipelineLayoutVK;
class DescriptorPoolVK;
class DescriptorSetLayoutVK;

class SkyboxRendererVK
{
public:
	SkyboxRendererVK(DeviceVK* pDevice);
	~SkyboxRendererVK();

	DECL_NO_COPY(SkyboxRendererVK);

	bool init();
	void generateCubemapFromPanorama(TextureCubeVK* pCubemap, Texture2DVK* pPanorama);

private:
	bool createCommandpoolsAndBuffers();
	bool createPipelineLayouts();
	bool createPipelines();
	bool createRenderPasses();

private:
	DeviceVK* m_pDevice;
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	DescriptorPoolVK* m_pDescriptorPool;

	RenderPassVK* m_pPanoramaRenderpass;
	SamplerVK* m_pPanoramaSampler;
	PipelineVK* m_pPanoramaPipeline;
	PipelineLayoutVK* m_pPanoramaPipelineLayout;
	DescriptorSetLayoutVK* m_pPanoramaDescriptorSetLayout;
	DescriptorSetVK* m_pPanoramaDescriptorSet;

	PipelineVK* m_pIrradiancePipeline;
	uint32_t m_CurrentFrame;
};