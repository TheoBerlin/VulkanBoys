#include "RayTracingRendererVK.h"

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

#include "ShaderBindingTableVK.h"
#include "RayTracingPipelineVK.h"

#include "Core/Application.h"

RayTracingRendererVK::RayTracingRendererVK(GraphicsContextVK* pContext, RenderingHandlerVK* pRenderingHandler) :
	m_pContext(pContext),
	m_pRenderingHandler(pRenderingHandler),
	m_pRayTracingPipeline(nullptr),
	m_pRayTracingDescriptorSet(nullptr),
	m_pRayTracingDescriptorPool(nullptr),
	m_pRayTracingDescriptorSetLayout(nullptr)
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppComputeCommandPools[i] = nullptr;
	}

	m_TempSubmitLimit = false;
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

	SAFEDELETE(m_pRayTracingUniformBuffer);

	SAFEDELETE(m_pRaygenShader);
	SAFEDELETE(m_pClosestHitShader);
	SAFEDELETE(m_pClosestHitShadowShader);
	SAFEDELETE(m_pMissShader);
	SAFEDELETE(m_pMissShadowShader);

	SAFEDELETE(m_pSampler);
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

	if (!createPipeline())
	{
		return false;
	}

	if (!createUniformBuffers())
	{
		return false;
	}


	SamplerParams samplerParams = {};
	samplerParams.MinFilter = VkFilter::VK_FILTER_LINEAR;
	samplerParams.MagFilter = VkFilter::VK_FILTER_LINEAR;
	samplerParams.WrapModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerParams.WrapModeV = samplerParams.WrapModeU;
	samplerParams.WrapModeW = samplerParams.WrapModeU;

	m_pSampler = new SamplerVK(m_pContext->getDevice());
	m_pSampler->init(samplerParams);

	createProfiler();

	return true;
}

void RayTracingRendererVK::beginFrame(IScene* pScene)
{
}

void RayTracingRendererVK::endFrame(IScene* pScene)
{
}

void RayTracingRendererVK::render(IScene* pScene, GBufferVK* pGBuffer)
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

	m_pProfiler->reset(currentFrame, m_ppComputeCommandBuffers[currentFrame]);
	m_pProfiler->beginFrame(m_ppComputeCommandBuffers[currentFrame]);

	CameraMatricesBuffer cameraMatricesBuffer = {};
	cameraMatricesBuffer.Projection = glm::inverse(pVulkanScene->getCamera().getProjectionMat());
	cameraMatricesBuffer.View = glm::inverse(pVulkanScene->getCamera().getViewMat());
	m_ppComputeCommandBuffers[currentFrame]->updateBuffer(m_pRayTracingUniformBuffer, 0, (const void*)&cameraMatricesBuffer, sizeof(CameraMatricesBuffer));

	//Temp
	//pVulkanScene->update();

	auto& allMaterials = pVulkanScene->getAllMaterials();

	std::vector<const ImageViewVK*> albedoImageViews;
	albedoImageViews.reserve(MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	std::vector<const ImageViewVK*> normalImageViews;
	normalImageViews.reserve(MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	std::vector<const ImageViewVK*> roughnessImageViews;
	roughnessImageViews.reserve(MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);

	std::vector<const SamplerVK*> samplers;
	samplers.reserve(MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);

	for (uint32_t i = 0; i < MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES; i++)
	{
		if (i < allMaterials.size())
		{
			ITexture2D* pAlbedoMap = allMaterials[i]->getAlbedoMap() != nullptr ? allMaterials[i]->getAlbedoMap() : allMaterials[0]->getAlbedoMap();
			ITexture2D* pNormalMap = allMaterials[i]->getNormalMap() != nullptr ? allMaterials[i]->getNormalMap() : allMaterials[0]->getNormalMap();
			ITexture2D* pRoughnessMap = allMaterials[i]->getRoughnessMap() != nullptr ? allMaterials[i]->getRoughnessMap() : allMaterials[0]->getRoughnessMap();

			albedoImageViews.push_back(reinterpret_cast<Texture2DVK*>(pAlbedoMap)->getImageView());
			normalImageViews.push_back(reinterpret_cast<Texture2DVK*>(pNormalMap)->getImageView());
			roughnessImageViews.push_back(reinterpret_cast<Texture2DVK*>(pRoughnessMap)->getImageView());
			samplers.push_back(reinterpret_cast<SamplerVK*>(allMaterials[i]->getSampler()));
		}
		else
		{
			albedoImageViews.push_back(reinterpret_cast<Texture2DVK*>(allMaterials[0]->getAlbedoMap())->getImageView());
			normalImageViews.push_back(reinterpret_cast<Texture2DVK*>(allMaterials[0]->getNormalMap())->getImageView());
			roughnessImageViews.push_back(reinterpret_cast<Texture2DVK*>(allMaterials[0]->getRoughnessMap())->getImageView());
			samplers.push_back(reinterpret_cast<SamplerVK*>(allMaterials[0]->getSampler()));
		}
	}

	ImageViewVK* pDepthImageView[1] = { pGBuffer->getDepthImageView() };
	ImageViewVK* pAlbedoImageView[1] = { pGBuffer->getColorAttachment(0) };
	ImageViewVK* pNormalImageView[1] = { pGBuffer->getColorAttachment(1) };
	
	m_pRayTracingDescriptorSet->writeAccelerationStructureDescriptor(pVulkanScene->getTLAS().accelerationStructure,									2);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(pAlbedoImageView, &m_pSampler, 1,														3);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(pNormalImageView, &m_pSampler, 1,														4);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(pDepthImageView, &m_pSampler, 1,														5);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(pVulkanScene->getCombinedVertexBuffer(),												6);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(pVulkanScene->getCombinedIndexBuffer(),												7);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(pVulkanScene->getMeshIndexBuffer(),													8);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(albedoImageViews.data(), samplers.data(), MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES,	9);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(normalImageViews.data(), samplers.data(), MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES,	10);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(roughnessImageViews.data(), samplers.data(), MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES, 11);
	

	vkCmdBindPipeline(m_ppComputeCommandBuffers[currentFrame]->getCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipeline->getPipeline());

	m_ppComputeCommandBuffers[currentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipelineLayout, 0, 1, &m_pRayTracingDescriptorSet, 0, nullptr);
	m_pProfiler->beginTimestamp(&m_TimestampTraceRays);
	m_ppComputeCommandBuffers[currentFrame]->traceRays(m_pRayTracingPipeline->getSBT(), m_pContext->getSwapChain()->getExtent().width, m_pContext->getSwapChain()->getExtent().height, 0);
	m_pProfiler->endTimestamp(&m_TimestampTraceRays);

	m_pProfiler->endFrame();
	m_ppComputeCommandBuffers[currentFrame]->end();
}

void RayTracingRendererVK::setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY)
{
	
}

void RayTracingRendererVK::onWindowResize(uint32_t width, uint32_t height)
{
}

void RayTracingRendererVK::setRayTracingResult(ImageViewVK* pRayTracingResultImageView)
{
	m_pRayTracingDescriptorSet->writeStorageImageDescriptor(pRayTracingResultImageView, 0);
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
		m_ppComputeCommandPools[i] = new CommandPoolVK(pDevice, computeQueueFamilyIndex);

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
	m_pRayTracingDescriptorSetLayout = new DescriptorSetLayoutVK(m_pContext->getDevice());
	//Result
	m_pRayTracingDescriptorSetLayout->addBindingStorageImage(VK_SHADER_STAGE_RAYGEN_BIT_NV,																		0, 1);

	//Uniform Buffer
	m_pRayTracingDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_MISS_BIT_NV, 1, 1);

	//TLAS
	m_pRayTracingDescriptorSetLayout->addBindingAccelerationStructure(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,						2, 1);

	//GBuffer
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, nullptr,															3, 1);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, nullptr,															4, 1);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, nullptr,															5, 1);

	//Scene Information
	m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,																6, 1);
	m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,																7, 1);
	m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,																8, 1);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr,														9,  MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr,														10, MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr,														11, MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	m_pRayTracingDescriptorSetLayout->finalize();

	std::vector<const DescriptorSetLayoutVK*> rayTracingDescriptorSetLayouts = { m_pRayTracingDescriptorSetLayout };
	std::vector<VkPushConstantRange> rayTracingPushConstantRanges;

	//Descriptorpool
	DescriptorCounts descriptorCounts = {};
	descriptorCounts.m_SampledImages = 256;
	descriptorCounts.m_StorageBuffers = 16;
	descriptorCounts.m_UniformBuffers = 16;
	descriptorCounts.m_StorageImages = 1;
	descriptorCounts.m_AccelerationStructures = 1;

	m_pRayTracingDescriptorPool = new DescriptorPoolVK(m_pContext->getDevice());
	m_pRayTracingDescriptorPool->init(descriptorCounts, 16);
	m_pRayTracingDescriptorSet = m_pRayTracingDescriptorPool->allocDescriptorSet(m_pRayTracingDescriptorSetLayout);
	if (m_pRayTracingDescriptorSet == nullptr)
	{
		return false;
	}

	m_pRayTracingPipelineLayout = new PipelineLayoutVK(m_pContext->getDevice());
	m_pRayTracingPipelineLayout->init(rayTracingDescriptorSetLayouts, rayTracingPushConstantRanges);

	return true;
}

bool RayTracingRendererVK::createPipeline()
{
	RaygenGroupParams raygenGroupParams = {};
	HitGroupParams hitGroupParams = {};
	HitGroupParams hitGroupShadowParams = {};
	MissGroupParams missGroupParams = {};
	MissGroupParams missGroupShadowParams = {};

	{
		m_pRaygenShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		m_pRaygenShader->initFromFile(EShader::RAYGEN_SHADER, "main", "assets/shaders/raytracing/raygen.spv");
		m_pRaygenShader->finalize();
		raygenGroupParams.pRaygenShader = m_pRaygenShader;

		m_pClosestHitShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		m_pClosestHitShader->initFromFile(EShader::CLOSEST_HIT_SHADER, "main", "assets/shaders/raytracing/closesthit.spv");
		m_pClosestHitShader->finalize();
		m_pClosestHitShader->setSpecializationConstant<uint32_t>(0, 3);
		hitGroupParams.pClosestHitShader = m_pClosestHitShader;

		m_pClosestHitShadowShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		m_pClosestHitShadowShader->initFromFile(EShader::CLOSEST_HIT_SHADER, "main", "assets/shaders/raytracing/closesthitShadow.spv");
		m_pClosestHitShadowShader->finalize();
		m_pClosestHitShadowShader->setSpecializationConstant<uint32_t>(0, 3);
		hitGroupShadowParams.pClosestHitShader = m_pClosestHitShadowShader;

		m_pMissShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		m_pMissShader->initFromFile(EShader::MISS_SHADER, "main", "assets/shaders/raytracing/miss.spv");
		m_pMissShader->finalize();
		missGroupParams.pMissShader = m_pMissShader;

		m_pMissShadowShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		m_pMissShadowShader->initFromFile(EShader::MISS_SHADER, "main", "assets/shaders/raytracing/missShadow.spv");
		m_pMissShadowShader->finalize();
		missGroupShadowParams.pMissShader = m_pMissShadowShader;
	}

	m_pRayTracingPipeline = new RayTracingPipelineVK(m_pContext);
	m_pRayTracingPipeline->addRaygenShaderGroup(raygenGroupParams);
	m_pRayTracingPipeline->addMissShaderGroup(missGroupParams);
	m_pRayTracingPipeline->addMissShaderGroup(missGroupShadowParams);
	m_pRayTracingPipeline->addHitShaderGroup(hitGroupParams);
	m_pRayTracingPipeline->addHitShaderGroup(hitGroupShadowParams);
	m_pRayTracingPipeline->finalize(m_pRayTracingPipelineLayout);

	return true;
}

bool RayTracingRendererVK::createUniformBuffers()
{
	BufferParams rayTracingUniformBufferParams = {};
	rayTracingUniformBufferParams.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	rayTracingUniformBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	rayTracingUniformBufferParams.SizeInBytes = sizeof(CameraMatricesBuffer);

	m_pRayTracingUniformBuffer = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pRayTracingUniformBuffer->init(rayTracingUniformBufferParams);

	m_pRayTracingDescriptorSet->writeUniformBufferDescriptor(m_pRayTracingUniformBuffer, 1);

	return true;
}

void RayTracingRendererVK::createProfiler()
{
	m_pProfiler = DBG_NEW ProfilerVK("Raytracer", m_pContext->getDevice());
	m_pProfiler->initTimestamp(&m_TimestampTraceRays, "Trace Rays");

	//ProfilerVK* pSceneProfiler = m_pRayTracingScene->getProfiler();
	//pSceneProfiler->setParentProfiler(m_pProfiler);
}
