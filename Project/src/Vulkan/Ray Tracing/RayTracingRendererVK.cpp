#include "RayTracingRendererVK.h"

#include <imgui/imgui.h>

#include "Core/Material.h"

#include "Vulkan/GraphicsContextVK.h"
#include "Vulkan/SwapChainVK.h"
#include "Vulkan/RenderingHandlerVK.h"
#include "Vulkan/PipelineLayoutVK.h"
#include "Vulkan/DescriptorSetLayoutVK.h"
#include "Vulkan/DescriptorPoolVK.h"
#include "Vulkan/DescriptorSetVK.h"
#include "Vulkan/CommandPoolVK.h"
#include "Vulkan/CommandBufferVK.h"
#include "Vulkan/ImageViewVK.h"
#include "Vulkan/ImageVK.h"
#include "Vulkan/SamplerVK.h"
#include "Vulkan/MeshVK.h"
#include "Vulkan/ShaderVK.h"
#include "Vulkan/BufferVK.h"
#include "Vulkan/FrameBufferVK.h"
#include "Vulkan/RenderPassVK.h"
#include "Vulkan/SceneVK.h"
#include "Vulkan/GBufferVK.h"
#include "Vulkan/TextureCubeVK.h"
#include "Vulkan/PipelineVK.h"

#include "ShaderBindingTableVK.h"
#include "RayTracingPipelineVK.h"

#include "Core/Application.h"

constexpr uint32_t MAX_RECURSIONS = 0;

RayTracingRendererVK::RayTracingRendererVK(GraphicsContextVK* pContext, RenderingHandlerVK* pRenderingHandler) :
	m_pContext(pContext),
	m_pRenderingHandler(pRenderingHandler),
	m_pRayTracingPipeline(nullptr),
	m_pRayTracingPipelineLayout(nullptr),
	m_pRayTracingDescriptorSet(nullptr),
	m_pRayTracingDescriptorPool(nullptr),
	m_pRayTracingDescriptorSetLayout(nullptr),
	m_pBlurPassPipeline(nullptr),
	m_pBlurPassPipelineLayout(nullptr),
	m_pBlurPassDescriptorPool(nullptr),
	m_pBlurPassDescriptorSetLayout(nullptr),
	m_pSkybox(nullptr),
	m_pBRDFLookUp(nullptr),
	m_pBlueNoise(nullptr),
	m_pCameraBuffer(nullptr),
	m_pLightsBuffer(nullptr),
	m_pReflectionFinalImage(nullptr),
	m_pReflectionFinalImageView(nullptr),
	m_pReflectionIntermediateImage(nullptr),
	m_pReflectionIntermediateImageView(nullptr),
	m_pReflectionTemporalAccumulationImage(nullptr),
	m_pReflectionTemporalAccumulationImageView(nullptr),
	m_pNearestSampler(nullptr),
	m_pLinearSampler(nullptr),
	m_RaysWidth(0),
	m_RaysHeight(0),
	m_NumBlurImagePixels(0),
	m_WorkGroupSize(),
	m_pHorizontalExtraBlurPassDescriptorSet(nullptr),
	m_pHorizontalInitialBlurPassDescriptorSet(nullptr),
	m_pVerticalBlurPassDescriptorSet(nullptr),
	m_ppComputeCommandBuffers(),
	m_ppComputeCommandPools(),
	m_pProfiler(nullptr)
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppComputeCommandPools[i] = nullptr;
	}
}

RayTracingRendererVK::~RayTracingRendererVK()
{
	SAFEDELETE(m_pProfiler);

	//Ray Tracing Stuff
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		SAFEDELETE(m_ppComputeCommandPools[i]);
	}

	SAFEDELETE(m_pRayTracingPipeline);
	SAFEDELETE(m_pRayTracingPipelineLayout);
	SAFEDELETE(m_pRayTracingDescriptorPool);
	SAFEDELETE(m_pRayTracingDescriptorSetLayout);

	SAFEDELETE(m_pBlurPassPipeline);
	SAFEDELETE(m_pBlurPassPipelineLayout);
	SAFEDELETE(m_pBlurPassDescriptorPool);
	SAFEDELETE(m_pBlurPassDescriptorSetLayout);

	SAFEDELETE(m_pCameraBuffer);
	SAFEDELETE(m_pLightsBuffer);

	SAFEDELETE(m_pNearestSampler);
	SAFEDELETE(m_pLinearSampler);

	SAFEDELETE(m_pReflectionTemporalAccumulationImage);
	SAFEDELETE(m_pReflectionTemporalAccumulationImageView);

	SAFEDELETE(m_pReflectionIntermediateImage);
	SAFEDELETE(m_pReflectionIntermediateImageView);

	SAFEDELETE(m_pBlueNoise);
}

bool RayTracingRendererVK::init()
{
	if (!createCommandPoolAndBuffers())
	{
		return false;
	}

	if (!createPipelineLayouts())
	{
		return false;
	}

	if (!createPipelines())
	{
		return false;
	}

	if (!createUniformBuffers())
	{
		return false;
	}

	if (!createSamplers())
	{
		return false;
	}

	if (!createTextures())
	{
		return false;
	}

	createProfiler();

	return true;
}

void RayTracingRendererVK::beginFrame(IScene*)
{
}

void RayTracingRendererVK::endFrame(IScene*)
{
}

void RayTracingRendererVK::render(IScene* pScene)
{
	SceneVK* pVulkanScene = reinterpret_cast<SceneVK*>(pScene);
	uint32_t currentFrame = m_pRenderingHandler->getCurrentFrameIndex();

	m_ppComputeCommandBuffers[currentFrame]->reset(false);
	m_ppComputeCommandPools[currentFrame]->reset();

	// Needed to begin a secondary buffer
	VkCommandBufferInheritanceInfo inheritanceInfo = {};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.pNext = nullptr;
	inheritanceInfo.renderPass = VK_NULL_HANDLE;
	inheritanceInfo.subpass = 0;
	inheritanceInfo.framebuffer = VK_NULL_HANDLE;

	m_ppComputeCommandBuffers[currentFrame]->begin(&inheritanceInfo, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	//Ray Trace
	{
		m_pProfiler->reset(currentFrame, m_ppComputeCommandBuffers[currentFrame]);
		m_pProfiler->beginFrame(m_ppComputeCommandBuffers[currentFrame]);

		updateBuffers(pVulkanScene, m_ppComputeCommandBuffers[currentFrame]);

		static float counter = 0.0f;
		counter += 0.01f;
		m_ppComputeCommandBuffers[currentFrame]->pushConstants(m_pRayTracingPipelineLayout, VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 0, sizeof(float), &counter);
		m_ppComputeCommandBuffers[currentFrame]->pushConstants(m_pRayTracingPipelineLayout, VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, sizeof(float), sizeof(GPURayTracingParameters), &m_GPURayTracingParameters);

		vkCmdBindPipeline(m_ppComputeCommandBuffers[currentFrame]->getCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipeline->getPipeline());

		m_ppComputeCommandBuffers[currentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipelineLayout, 0, 1, &m_pRayTracingDescriptorSet, 0, nullptr);
		
		m_ppComputeCommandBuffers[currentFrame]->transitionImageLayout(m_pReflectionTemporalAccumulationImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1);
		
		m_pProfiler->beginTimestamp(&m_TimestampTraceRays);
		m_ppComputeCommandBuffers[currentFrame]->traceRays(m_pRayTracingPipeline->getSBT(), m_RaysWidth, m_RaysHeight, 0);
		m_pProfiler->endTimestamp(&m_TimestampTraceRays);

		m_pProfiler->endFrame();
	}

	//constexpr uint32_t NUM_BLUR_PASSES = 8;

	if (m_CPURayTracingParameters.NumBlurPasses > 0)
	{
		glm::u32vec3 workGroupSize(1 + m_NumBlurImagePixels / m_WorkGroupSize[0], 1, 1);

		m_ppComputeCommandBuffers[currentFrame]->bindPipeline(m_pBlurPassPipeline);

 		m_ppComputeCommandBuffers[currentFrame]->pushConstants(m_pBlurPassPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 2 * sizeof(float), sizeof(GPURayTracingParameters), &m_GPURayTracingParameters);

		//Initial Blur Pass
		{
			//Horizontal Blur
			{
				constexpr float BLUR_DIRECTION[2] = { 1.0f, 0.0f };
				//Read Final
				//Write Intermediate
				m_ppComputeCommandBuffers[currentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, m_pBlurPassPipelineLayout, 0, 1, &m_pHorizontalInitialBlurPassDescriptorSet, 0, nullptr);

				m_ppComputeCommandBuffers[currentFrame]->transitionImageLayout(m_pReflectionTemporalAccumulationImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);
				m_ppComputeCommandBuffers[currentFrame]->transitionImageLayout(m_pReflectionIntermediateImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1);

				m_ppComputeCommandBuffers[currentFrame]->pushConstants(m_pBlurPassPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 2 * sizeof(float), &BLUR_DIRECTION);

				m_ppComputeCommandBuffers[currentFrame]->dispatch(workGroupSize);
			}

			//Vertical Blur
			{
				constexpr float BLUR_DIRECTION[2] = { 0.0f, 1.0f };
				//Write Final
				//Read Intermediate
				m_ppComputeCommandBuffers[currentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, m_pBlurPassPipelineLayout, 0, 1, &m_pVerticalBlurPassDescriptorSet, 0, nullptr);

				m_ppComputeCommandBuffers[currentFrame]->transitionImageLayout(m_pReflectionIntermediateImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);

				m_ppComputeCommandBuffers[currentFrame]->pushConstants(m_pBlurPassPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 2 * sizeof(float), &BLUR_DIRECTION);

				m_ppComputeCommandBuffers[currentFrame]->dispatch(workGroupSize);
			}
		}

		//Extra Blur Passes
		if (m_CPURayTracingParameters.NumBlurPasses > 1)
		{

			for (uint32_t blurPass = 1; blurPass < (uint32_t)m_CPURayTracingParameters.NumBlurPasses; blurPass++)
			{
				//Horizontal Blur
				{
					constexpr float BLUR_DIRECTION[2] = { 1.0f, 0.0f };
					//Read Final
					//Write Intermediate
					m_ppComputeCommandBuffers[currentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, m_pBlurPassPipelineLayout, 0, 1, &m_pHorizontalExtraBlurPassDescriptorSet, 0, nullptr);

					m_ppComputeCommandBuffers[currentFrame]->transitionImageLayout(m_pReflectionFinalImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);
					m_ppComputeCommandBuffers[currentFrame]->transitionImageLayout(m_pReflectionIntermediateImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1);

					m_ppComputeCommandBuffers[currentFrame]->pushConstants(m_pBlurPassPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 2 * sizeof(float), &BLUR_DIRECTION);

					m_ppComputeCommandBuffers[currentFrame]->dispatch(workGroupSize);
				}

				//Vertical Blur
				{
					constexpr float BLUR_DIRECTION[2] = { 0.0f, 1.0f };
					//Write Final
					//Read Intermediate
					m_ppComputeCommandBuffers[currentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, m_pBlurPassPipelineLayout, 0, 1, &m_pVerticalBlurPassDescriptorSet, 0, nullptr);

					m_ppComputeCommandBuffers[currentFrame]->transitionImageLayout(m_pReflectionIntermediateImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);
					m_ppComputeCommandBuffers[currentFrame]->transitionImageLayout(m_pReflectionFinalImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1);

					m_ppComputeCommandBuffers[currentFrame]->pushConstants(m_pBlurPassPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 2 * sizeof(float), &BLUR_DIRECTION);

					m_ppComputeCommandBuffers[currentFrame]->dispatch(workGroupSize);
				}
			}
		}
	}

	m_ppComputeCommandBuffers[currentFrame]->end();
}

void RayTracingRendererVK::renderUI()
{
	ImGui::SliderFloat("Max Temporal Frames", &m_GPURayTracingParameters.MaxTemporalFrames, 1.0f, 1024.0f);
	ImGui::SliderFloat("Min Temporal Weight", &m_GPURayTracingParameters.MinTemporalWeight, 0.0000001f, 1.0f, "%.3f", 2.0f);
	ImGui::SliderFloat("Reflection Ray Bias", &m_GPURayTracingParameters.ReflectionRayBias, 0.0f, 0.5f);
	ImGui::SliderFloat("Shadow Ray Bias", &m_GPURayTracingParameters.ShadowRayBias, 0.0f, 0.5f);

	ImGui::SliderInt("Num Spatial Filter Passes", &m_CPURayTracingParameters.NumBlurPasses, 1, 16);
}

void RayTracingRendererVK::setViewport(float, float, float, float, float, float)
{
	
}

void RayTracingRendererVK::onWindowResize(uint32_t width, uint32_t height)
{
	if (m_RaysWidth != width || m_RaysHeight != height)
	{
		SAFEDELETE(m_pReflectionTemporalAccumulationImage);
		SAFEDELETE(m_pReflectionTemporalAccumulationImageView);
		SAFEDELETE(m_pReflectionIntermediateImage);
		SAFEDELETE(m_pReflectionIntermediateImageView);

		m_RaysWidth = width;
		m_RaysHeight = height;

		ImageParams imageParams = {};
		imageParams.Type = VK_IMAGE_TYPE_2D;
		imageParams.Format = VK_FORMAT_R16G16B16A16_SFLOAT; //Todo: What format should this be?
		imageParams.Extent.width = m_RaysWidth;
		imageParams.Extent.height = m_RaysHeight;
		imageParams.Extent.depth = 1;
		imageParams.MipLevels = 1;
		imageParams.ArrayLayers = 1;
		imageParams.Samples = VK_SAMPLE_COUNT_1_BIT;
		imageParams.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		m_pReflectionTemporalAccumulationImage = DBG_NEW ImageVK(m_pContext->getDevice());
		m_pReflectionTemporalAccumulationImage->init(imageParams);

		m_pReflectionIntermediateImage = DBG_NEW ImageVK(m_pContext->getDevice());
		m_pReflectionIntermediateImage->init(imageParams);

		ImageViewParams imageViewParams = {};
		imageViewParams.Type = VK_IMAGE_VIEW_TYPE_2D;
		imageViewParams.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewParams.FirstMipLevel = 0;
		imageViewParams.MipLevels = 1;
		imageViewParams.FirstLayer = 0;
		imageViewParams.LayerCount = 1;

		m_pReflectionTemporalAccumulationImageView = DBG_NEW ImageViewVK(m_pContext->getDevice(), m_pReflectionTemporalAccumulationImage);
		m_pReflectionTemporalAccumulationImageView->init(imageViewParams);

		m_pReflectionIntermediateImageView = DBG_NEW ImageViewVK(m_pContext->getDevice(), m_pReflectionIntermediateImage);
		m_pReflectionIntermediateImageView->init(imageViewParams);

		CommandBufferVK* pTempCommandBuffer = m_ppComputeCommandPools[0]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		pTempCommandBuffer->reset(true);
		pTempCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		pTempCommandBuffer->transitionImageLayout(m_pReflectionTemporalAccumulationImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);
		pTempCommandBuffer->transitionImageLayout(m_pReflectionIntermediateImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);
		pTempCommandBuffer->end();

		m_pContext->getDevice()->executeCompute(pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
		m_pContext->getDevice()->wait(); //Todo: Remove This

		m_ppComputeCommandPools[0]->freeCommandBuffer(&pTempCommandBuffer);

		//Update Descriptor Sets
		m_pRayTracingDescriptorSet->writeStorageImageDescriptor(m_pReflectionTemporalAccumulationImageView, RT_RAW_REFLECTION_IMAGE_BINDING);

		m_pHorizontalInitialBlurPassDescriptorSet->writeCombinedImageDescriptors(&m_pReflectionTemporalAccumulationImageView, &m_pLinearSampler, 1, RT_BP_INPUT_BINDING);

		m_pHorizontalInitialBlurPassDescriptorSet->writeStorageImageDescriptor(m_pReflectionIntermediateImageView, RT_BP_OUTPUT_BINDING);
		m_pHorizontalExtraBlurPassDescriptorSet->writeStorageImageDescriptor(m_pReflectionIntermediateImageView, RT_BP_OUTPUT_BINDING);

		m_pVerticalBlurPassDescriptorSet->writeCombinedImageDescriptors(&m_pReflectionIntermediateImageView, &m_pLinearSampler, 1, RT_BP_INPUT_BINDING);
	}
}

void RayTracingRendererVK::setRayTracingResultTextures(ImageVK*, ImageViewVK* pRadianceImageView, ImageVK* pGlossyImage, ImageViewVK* pGlossyImageView, uint32_t width, uint32_t height)
{
	m_NumBlurImagePixels = width * height;
	m_pRayTracingDescriptorSet->writeStorageImageDescriptor(pRadianceImageView, RT_RADIANCE_IMAGE_BINDING);

	m_pReflectionFinalImage = pGlossyImage;
	m_pReflectionFinalImageView = pGlossyImageView;

	m_pHorizontalExtraBlurPassDescriptorSet->writeCombinedImageDescriptors(&m_pReflectionFinalImageView, &m_pLinearSampler, 1, RT_BP_INPUT_BINDING);

	m_pVerticalBlurPassDescriptorSet->writeStorageImageDescriptor(m_pReflectionFinalImageView, RT_BP_OUTPUT_BINDING);
}

void RayTracingRendererVK::setSkybox(TextureCubeVK* pSkybox)
{
	m_pSkybox = pSkybox;

	ImageViewVK* pSkyboxView = m_pSkybox->getImageView();
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(&pSkyboxView, &m_pNearestSampler, 1, RT_SKYBOX_BINDING);	
}

void RayTracingRendererVK::setGBufferTextures(GBufferVK* pGBuffer)
{
	ImageViewVK* pDepthImageView[1] = { pGBuffer->getDepthImageView() };
	ImageViewVK* pAlbedoImageView[1] = { pGBuffer->getColorAttachment(0) };
	ImageViewVK* pNormalImageView[1] = { pGBuffer->getColorAttachment(1) };
	ImageViewVK* pVelocityImageView[1] = { pGBuffer->getColorAttachment(2) };

	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(pAlbedoImageView, &m_pNearestSampler, 1, RT_GBUFFER_ALBEDO_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(pNormalImageView, &m_pNearestSampler, 1, RT_GBUFFER_NORMAL_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(pDepthImageView, &m_pNearestSampler, 1, RT_GBUFFER_DEPTH_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(pVelocityImageView, &m_pNearestSampler, 1, RT_GBUFFER_VELOCITY_BINDING);

	m_pHorizontalInitialBlurPassDescriptorSet->writeCombinedImageDescriptors(pAlbedoImageView, &m_pNearestSampler, 1, RT_BP_GBUFFER_ALBEDO_BINDING);
	m_pHorizontalInitialBlurPassDescriptorSet->writeCombinedImageDescriptors(pNormalImageView, &m_pNearestSampler, 1, RT_BP_GBUFFER_NORMAL_BINDING);
	m_pHorizontalInitialBlurPassDescriptorSet->writeCombinedImageDescriptors(pDepthImageView, &m_pNearestSampler, 1, RT_BP_GBUFFER_DEPTH_BINDING);

	m_pVerticalBlurPassDescriptorSet->writeCombinedImageDescriptors(pAlbedoImageView, &m_pNearestSampler, 1, RT_BP_GBUFFER_ALBEDO_BINDING);
	m_pVerticalBlurPassDescriptorSet->writeCombinedImageDescriptors(pNormalImageView, &m_pNearestSampler, 1, RT_BP_GBUFFER_NORMAL_BINDING);
	m_pVerticalBlurPassDescriptorSet->writeCombinedImageDescriptors(pDepthImageView, &m_pNearestSampler, 1, RT_BP_GBUFFER_DEPTH_BINDING);

	m_pHorizontalExtraBlurPassDescriptorSet->writeCombinedImageDescriptors(pAlbedoImageView, &m_pNearestSampler, 1, RT_BP_GBUFFER_ALBEDO_BINDING);
	m_pHorizontalExtraBlurPassDescriptorSet->writeCombinedImageDescriptors(pNormalImageView, &m_pNearestSampler, 1, RT_BP_GBUFFER_NORMAL_BINDING);
	m_pHorizontalExtraBlurPassDescriptorSet->writeCombinedImageDescriptors(pDepthImageView, &m_pNearestSampler, 1, RT_BP_GBUFFER_DEPTH_BINDING);
}

void RayTracingRendererVK::setSceneData(IScene* pScene)
{
	SceneVK* pVulkanScene = reinterpret_cast<SceneVK*>(pScene);

	const std::vector<const ImageViewVK*>& albedoMaps = pVulkanScene->getAlbedoMaps();
	const std::vector<const ImageViewVK*>& normalMaps = pVulkanScene->getNormalMaps();
	const std::vector<const ImageViewVK*>& aoMaps = pVulkanScene->getAOMaps();
	const std::vector<const ImageViewVK*>& metallicMaps = pVulkanScene->getMetallicMaps();
	const std::vector<const ImageViewVK*>& roughnessMaps = pVulkanScene->getRoughnessMaps();
	const std::vector<const SamplerVK*>& samplers = pVulkanScene->getSamplers();
	const BufferVK* pMaterialParametersBuffer = pVulkanScene->getMaterialParametersBuffer();

	m_pRayTracingDescriptorSet->writeAccelerationStructureDescriptor(pVulkanScene->getTLAS().AccelerationStructure, RT_TLAS_BINDING);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(pVulkanScene->getCombinedVertexBuffer(), RT_COMBINED_VERTEX_BINDING);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(pVulkanScene->getCombinedIndexBuffer(), RT_COMBINED_INDEX_BINDING);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(pVulkanScene->getMeshIndexBuffer(), RT_MESH_INDEX_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(albedoMaps.data(), samplers.data(), MAX_NUM_UNIQUE_MATERIALS, RT_COMBINED_ALBEDO_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(normalMaps.data(), samplers.data(), MAX_NUM_UNIQUE_MATERIALS, RT_COMBINED_NORMAL_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(aoMaps.data(), samplers.data(), MAX_NUM_UNIQUE_MATERIALS, RT_COMBINED_AO_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(metallicMaps.data(), samplers.data(), MAX_NUM_UNIQUE_MATERIALS, RT_COMBINED_METALLIC_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(roughnessMaps.data(), samplers.data(), MAX_NUM_UNIQUE_MATERIALS, RT_COMBINED_ROUGHNESS_BINDING);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(pMaterialParametersBuffer, RT_COMBINED_MATERIAL_PARAMETERS_BINDING);
}

void RayTracingRendererVK::setBRDFLookUp(Texture2DVK* pTexture)
{
	m_pBRDFLookUp = pTexture;

	ImageViewVK* pBRDFLookUp = m_pBRDFLookUp->getImageView();
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(&pBRDFLookUp, &m_pNearestSampler, 1, RT_BRDF_LUT_BINDING);
}

CommandBufferVK* RayTracingRendererVK::getComputeCommandBuffer() const
{
	return m_ppComputeCommandBuffers[m_pRenderingHandler->getCurrentFrameIndex()];
}

bool RayTracingRendererVK::createCommandPoolAndBuffers()
{
	DeviceVK* pDevice = m_pContext->getDevice();
	
	const uint32_t computeQueueFamilyIndex = pDevice->getQueueFamilyIndices().computeFamily.value();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppComputeCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, computeQueueFamilyIndex);

		if (!m_ppComputeCommandPools[i]->init())
		{
			return false;
		}

		m_ppComputeCommandBuffers[i] = m_ppComputeCommandPools[i]->allocateCommandBuffer(VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_SECONDARY);
		if (m_ppComputeCommandBuffers[i] == nullptr)
		{
			return false;
		}
	}

	return true;
}

bool RayTracingRendererVK::createPipelineLayouts()
{
	//Ray Tracing
	{
		m_pRayTracingDescriptorSetLayout = DBG_NEW DescriptorSetLayoutVK(m_pContext->getDevice());

		//Result
		m_pRayTracingDescriptorSetLayout->addBindingStorageImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, RT_RADIANCE_IMAGE_BINDING, 1);
		m_pRayTracingDescriptorSetLayout->addBindingStorageImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, RT_RAW_REFLECTION_IMAGE_BINDING, 1);

		//Uniform Buffer
		m_pRayTracingDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_MISS_BIT_NV, RT_CAMERA_BUFFER_BINDING, 1);

		//TLAS
		m_pRayTracingDescriptorSetLayout->addBindingAccelerationStructure(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, RT_TLAS_BINDING, 1);

		//GBuffer
		m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, nullptr, RT_GBUFFER_ALBEDO_BINDING, 1);
		m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, nullptr, RT_GBUFFER_NORMAL_BINDING, 1);
		m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, nullptr, RT_GBUFFER_DEPTH_BINDING, 1);
		m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, nullptr, RT_GBUFFER_VELOCITY_BINDING, 1);

		//Scene Mesh Information
		m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, RT_COMBINED_VERTEX_BINDING, 1);
		m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, RT_COMBINED_INDEX_BINDING, 1);
		m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, RT_MESH_INDEX_BINDING, 1);

		//Scene Material Information
		m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr, RT_COMBINED_ALBEDO_BINDING, MAX_NUM_UNIQUE_MATERIALS);
		m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr, RT_COMBINED_NORMAL_BINDING, MAX_NUM_UNIQUE_MATERIALS);
		m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr, RT_COMBINED_AO_BINDING, MAX_NUM_UNIQUE_MATERIALS);
		m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr, RT_COMBINED_METALLIC_BINDING, MAX_NUM_UNIQUE_MATERIALS);
		m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr, RT_COMBINED_ROUGHNESS_BINDING, MAX_NUM_UNIQUE_MATERIALS);
		m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, RT_COMBINED_MATERIAL_PARAMETERS_BINDING, 1);

		//Cubemap
		m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_MISS_BIT_NV, nullptr, RT_SKYBOX_BINDING, 1);

		//Light Buffer
		m_pRayTracingDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, RT_LIGHT_BUFFER_BINDING, 1);

		//Look Ups
		m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr, RT_BRDF_LUT_BINDING, 1);
		m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr, RT_BLUE_NOISE_LOOKUP_BINDING, 1);

		m_pRayTracingDescriptorSetLayout->finalize();

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(float) + sizeof(GPURayTracingParameters);
		pushConstantRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

		std::vector<const DescriptorSetLayoutVK*> rayTracingDescriptorSetLayouts = { m_pRayTracingDescriptorSetLayout };
		std::vector<VkPushConstantRange> rayTracingPushConstantRanges = { pushConstantRange };

		//Descriptorpool
		DescriptorCounts descriptorCounts = {};
		descriptorCounts.m_SampledImages = MAX_NUM_UNIQUE_MATERIALS * 6;
		descriptorCounts.m_StorageBuffers = 16;
		descriptorCounts.m_UniformBuffers = 16;
		descriptorCounts.m_StorageImages = 2;
		descriptorCounts.m_AccelerationStructures = 1;

		m_pRayTracingDescriptorPool = DBG_NEW DescriptorPoolVK(m_pContext->getDevice());
		m_pRayTracingDescriptorPool->init(descriptorCounts, 256);
		m_pRayTracingDescriptorSet = m_pRayTracingDescriptorPool->allocDescriptorSet(m_pRayTracingDescriptorSetLayout);
		if (m_pRayTracingDescriptorSet == nullptr)
		{
			return false;
		}

		m_pRayTracingPipelineLayout = DBG_NEW PipelineLayoutVK(m_pContext->getDevice());
		m_pRayTracingPipelineLayout->init(rayTracingDescriptorSetLayouts, rayTracingPushConstantRanges);
	}

	//Blur Passes
	{
		m_pBlurPassDescriptorSetLayout = DBG_NEW DescriptorSetLayoutVK(m_pContext->getDevice());

		//Input/Output Images
		m_pBlurPassDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_COMPUTE_BIT, nullptr, RT_BP_INPUT_BINDING, 1);
		m_pBlurPassDescriptorSetLayout->addBindingStorageImage(VK_SHADER_STAGE_COMPUTE_BIT, RT_BP_OUTPUT_BINDING, 1);

		//GBuffer
		m_pBlurPassDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_COMPUTE_BIT, nullptr, RT_BP_GBUFFER_ALBEDO_BINDING, 1);
		m_pBlurPassDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_COMPUTE_BIT, nullptr, RT_BP_GBUFFER_NORMAL_BINDING, 1);
		m_pBlurPassDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_COMPUTE_BIT, nullptr, RT_BP_GBUFFER_DEPTH_BINDING, 1);

		m_pBlurPassDescriptorSetLayout->finalize();

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.offset = 0;
		pushConstantRange.size = 2 * sizeof(float) + sizeof(GPURayTracingParameters);
		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		std::vector<const DescriptorSetLayoutVK*> blurPassDescriptorSetLayouts = { m_pBlurPassDescriptorSetLayout };
		std::vector<VkPushConstantRange> blurPassPushConstantRanges = { pushConstantRange };

		//Descriptorpool
		constexpr uint32_t NUM_BLUR_DESCRIPTOR_SETS = 3;
		DescriptorCounts descriptorCounts = {};
		descriptorCounts.m_SampledImages = 4 * NUM_BLUR_DESCRIPTOR_SETS;
		descriptorCounts.m_StorageBuffers = 1;
		descriptorCounts.m_UniformBuffers = 1;
		descriptorCounts.m_StorageImages = 1 * NUM_BLUR_DESCRIPTOR_SETS;
		descriptorCounts.m_AccelerationStructures = 1;

		m_pBlurPassDescriptorPool = DBG_NEW DescriptorPoolVK(m_pContext->getDevice());
		m_pBlurPassDescriptorPool->init(descriptorCounts, 16);

		m_pHorizontalInitialBlurPassDescriptorSet = m_pBlurPassDescriptorPool->allocDescriptorSet(m_pBlurPassDescriptorSetLayout);
		if (m_pHorizontalInitialBlurPassDescriptorSet == nullptr)
		{
			return false;
		}

		m_pHorizontalExtraBlurPassDescriptorSet = m_pBlurPassDescriptorPool->allocDescriptorSet(m_pBlurPassDescriptorSetLayout);
		if (m_pHorizontalExtraBlurPassDescriptorSet == nullptr)
		{
			return false;
		}

		m_pVerticalBlurPassDescriptorSet = m_pBlurPassDescriptorPool->allocDescriptorSet(m_pBlurPassDescriptorSetLayout);
		if (m_pVerticalBlurPassDescriptorSet == nullptr)
		{
			return false;
		}

		m_pBlurPassPipelineLayout = DBG_NEW PipelineLayoutVK(m_pContext->getDevice());
		m_pBlurPassPipelineLayout->init(blurPassDescriptorSetLayouts, blurPassPushConstantRanges);
	}

	return true;
}

bool RayTracingRendererVK::createPipelines()
{
	//Ray Tracing
	{
		RaygenGroupParams raygenGroupParams = {};
		HitGroupParams hitGroupParams = {};
		HitGroupParams hitGroupShadowParams = {};
		MissGroupParams missGroupParams = {};
		MissGroupParams missGroupShadowParams = {};

		ShaderVK* pRaygenShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		pRaygenShader->initFromFile(EShader::RAYGEN_SHADER, "main", "assets/shaders/raytracing/raygen.spv");
		pRaygenShader->finalize();
		raygenGroupParams.pRaygenShader = pRaygenShader;

		ShaderVK* pClosestHitShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		pClosestHitShader->initFromFile(EShader::CLOSEST_HIT_SHADER, "main", "assets/shaders/raytracing/closesthit.spv");
		pClosestHitShader->finalize();
		pClosestHitShader->setSpecializationConstant<uint32_t>(0, MAX_RECURSIONS);
		pClosestHitShader->setSpecializationConstant<uint32_t>(1, MAX_NUM_UNIQUE_MATERIALS);
		hitGroupParams.pClosestHitShader = pClosestHitShader;

		ShaderVK* pClosestHitShadowShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		pClosestHitShadowShader->initFromFile(EShader::CLOSEST_HIT_SHADER, "main", "assets/shaders/raytracing/closesthitShadow.spv");
		pClosestHitShadowShader->finalize();
		hitGroupShadowParams.pClosestHitShader = pClosestHitShadowShader;

		ShaderVK* pMissShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		pMissShader->initFromFile(EShader::MISS_SHADER, "main", "assets/shaders/raytracing/miss.spv");
		pMissShader->finalize();
		missGroupParams.pMissShader = pMissShader;

		ShaderVK* pMissShadowShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		pMissShadowShader->initFromFile(EShader::MISS_SHADER, "main", "assets/shaders/raytracing/missShadow.spv");
		pMissShadowShader->finalize();
		missGroupShadowParams.pMissShader = pMissShadowShader;

		m_pRayTracingPipeline = DBG_NEW RayTracingPipelineVK(m_pContext);
		m_pRayTracingPipeline->addRaygenShaderGroup(raygenGroupParams);
		m_pRayTracingPipeline->addMissShaderGroup(missGroupParams);
		m_pRayTracingPipeline->addMissShaderGroup(missGroupShadowParams);
		m_pRayTracingPipeline->addHitShaderGroup(hitGroupParams);
		m_pRayTracingPipeline->addHitShaderGroup(hitGroupShadowParams);
		m_pRayTracingPipeline->finalize(m_pRayTracingPipelineLayout);

		SAFEDELETE(pRaygenShader);
		SAFEDELETE(pClosestHitShader);
		SAFEDELETE(pClosestHitShadowShader);
		SAFEDELETE(pMissShader);
		SAFEDELETE(pMissShadowShader);
	}

	//Blur Passes
	{
		DeviceVK* pDevice = m_pContext->getDevice();
		pDevice->getMaxComputeWorkGroupSize(m_WorkGroupSize);

		ShaderVK* pComputeShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		pComputeShader->initFromFile(EShader::COMPUTE_SHADER, "main", "assets/shaders/raytracing/blur.spv");
		pComputeShader->finalize();
		pComputeShader->setSpecializationConstant<int32_t>(0, m_WorkGroupSize[0]);

		m_pBlurPassPipeline = DBG_NEW PipelineVK(pDevice);
		m_pBlurPassPipeline->finalizeCompute(pComputeShader, m_pBlurPassPipelineLayout);

		SAFEDELETE(pComputeShader);
	}

	return true;
}

bool RayTracingRendererVK::createUniformBuffers()
{
	BufferParams cameraBufferParams = {};
	cameraBufferParams.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	cameraBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	cameraBufferParams.SizeInBytes = sizeof(CameraMatricesBuffer);

	m_pCameraBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pCameraBuffer->init(cameraBufferParams);
	m_pRayTracingDescriptorSet->writeUniformBufferDescriptor(m_pCameraBuffer, RT_CAMERA_BUFFER_BINDING);

	BufferParams lightsBufferParams = {};
	lightsBufferParams.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	lightsBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	lightsBufferParams.SizeInBytes = sizeof(PointLight) * MAX_POINTLIGHTS;

	m_pLightsBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pLightsBuffer->init(lightsBufferParams);
	m_pRayTracingDescriptorSet->writeUniformBufferDescriptor(m_pLightsBuffer, RT_LIGHT_BUFFER_BINDING);

	return true;
}

bool RayTracingRendererVK::createSamplers()
{
	bool result = true;
	SamplerParams nearestSamplerParams = {};
	nearestSamplerParams.MagFilter = VK_FILTER_NEAREST;
	nearestSamplerParams.MinFilter = VK_FILTER_NEAREST;
	nearestSamplerParams.WrapModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	nearestSamplerParams.WrapModeV = nearestSamplerParams.WrapModeU;
	nearestSamplerParams.WrapModeW = nearestSamplerParams.WrapModeU;

	m_pNearestSampler = DBG_NEW SamplerVK(m_pContext->getDevice());
	result = result && m_pNearestSampler->init(nearestSamplerParams);

	SamplerParams linearSamplerParams = {};
	linearSamplerParams.MagFilter = VK_FILTER_LINEAR;
	linearSamplerParams.MinFilter = VK_FILTER_LINEAR;
	linearSamplerParams.WrapModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	linearSamplerParams.WrapModeV = linearSamplerParams.WrapModeU;
	linearSamplerParams.WrapModeW = linearSamplerParams.WrapModeU;

	m_pLinearSampler = DBG_NEW SamplerVK(m_pContext->getDevice());
	result = result && m_pLinearSampler->init(linearSamplerParams);
	return result;
}

bool RayTracingRendererVK::createTextures()
{
	bool result = true;
	m_pBlueNoise = DBG_NEW Texture2DVK(m_pContext->getDevice());
	result = m_pBlueNoise->initFromFile("assets/textures/blue_noise/512_512/LDR_RG01_0.png", ETextureFormat::FORMAT_R8G8B8A8_UNORM, false);

	ImageViewVK* pBlueNoiseImageView = m_pBlueNoise->getImageView();
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(&pBlueNoiseImageView, &m_pLinearSampler, 1, RT_BLUE_NOISE_LOOKUP_BINDING);
	return result;
}

void RayTracingRendererVK::createProfiler()
{
	m_pProfiler = DBG_NEW ProfilerVK("Raytracer", m_pContext->getDevice());
	m_pProfiler->initTimestamp(&m_TimestampTraceRays, "Trace Rays");

	//ProfilerVK* pSceneProfiler = m_pRayTracingScene->getProfiler();
	//pSceneProfiler->setParentProfiler(m_pProfiler);
}

void RayTracingRendererVK::updateBuffers(SceneVK* pScene, CommandBufferVK* pCommandBuffer)
{
	//Update Camera
	CameraMatricesBuffer cameraMatricesBuffer = {};
	cameraMatricesBuffer.ProjectionInv = pScene->getCamera().getProjectionInvMat();
	cameraMatricesBuffer.ViewInv = pScene->getCamera().getViewInvMat();
	pCommandBuffer->updateBuffer(m_pCameraBuffer, 0, (const void*)&cameraMatricesBuffer, sizeof(CameraMatricesBuffer));

	//Update Lights
	const uint32_t numPointLights = pScene->getLightSetup().getPointLightCount();
	pCommandBuffer->updateBuffer(m_pLightsBuffer, 0, (const void*)pScene->getLightSetup().getPointLights(), sizeof(PointLight) * numPointLights);
}