#pragma once
#include "VulkanCommon.h"

class DeviceVK;
class BufferVK;
class InstanceVK;
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
	void generateIrradiance(TextureCubeVK* pCubemap, TextureCubeVK* pIrradianceMap);
	void prefilterEnvironmentMap(TextureCubeVK* pCubemap, TextureCubeVK* pEnvironmentMap);

private:
	bool createCommandpoolsAndBuffers();
	bool createPipelineLayouts();
	bool createPipelines();
	bool createRenderPasses();

private:
	DeviceVK* m_pDevice;
	CommandPoolVK* m_ppCommandPools[MAX_FRAMES_IN_FLIGHT];
	CommandBufferVK* m_ppCommandBuffers[MAX_FRAMES_IN_FLIGHT];

	BufferVK* m_pCubeFilterBuffer;
	SamplerVK* m_pCubeFilterSampler;
	RenderPassVK* m_pFilterCubeRenderpass;
	DescriptorPoolVK* m_pDescriptorPool;
	PipelineLayoutVK* m_pFilterCubePipelineLayout;
	DescriptorSetLayoutVK* m_pFilterCubeDescriptorSetLayout;

	PipelineVK* m_pPanoramaPipeline;
	DescriptorSetVK* m_pPanoramaDescriptorSet;

	PipelineVK* m_pIrradiancePipeline;
	DescriptorSetVK* m_pIrradianceDescriptorSet;

	PipelineVK* m_pPreFilterPipeline;
	DescriptorSetVK* m_pPreFilterDescriptorSet;

	uint32_t m_CurrentFrame;
};