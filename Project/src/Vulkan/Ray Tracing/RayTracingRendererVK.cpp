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
#include "Vulkan/TextureCubeVK.h"

#include "ShaderBindingTableVK.h"
#include "RayTracingPipelineVK.h"

#include "Core/Application.h"

constexpr uint32_t MAX_RECURSIONS = 1;

RayTracingRendererVK::RayTracingRendererVK(GraphicsContextVK* pContext, RenderingHandlerVK* pRenderingHandler) :
	m_pContext(pContext),
	m_pRenderingHandler(pRenderingHandler),
	m_pRayTracingPipeline(nullptr),
	m_pRayTracingDescriptorSet(nullptr),
	m_pRayTracingDescriptorPool(nullptr),
	m_pRayTracingDescriptorSetLayout(nullptr),
	m_pCameraBuffer(nullptr),
	m_pLightsBuffer(nullptr),
	m_RaysWidth(0),
	m_RaysHeight(0)
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

	SAFEDELETE(m_pCameraBuffer);
	SAFEDELETE(m_pLightsBuffer);

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
	samplerParams.MagFilter = VK_FILTER_NEAREST;
	samplerParams.MinFilter = VK_FILTER_NEAREST;
	samplerParams.WrapModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
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

	updateBuffers(pVulkanScene, m_ppComputeCommandBuffers[currentFrame]);

	//Temp
	//pVulkanScene->update();

	const std::vector<const ImageViewVK*>& albedoMaps = pVulkanScene->getAlbedoMaps();
	const std::vector<const ImageViewVK*>& normalMaps = pVulkanScene->getNormalMaps();
	const std::vector<const ImageViewVK*>& aoMaps = pVulkanScene->getAOMaps();
	const std::vector<const ImageViewVK*>& metallicMaps = pVulkanScene->getMetallicMaps();
	const std::vector<const ImageViewVK*>& roughnessMaps = pVulkanScene->getRoughnessMaps();
	const std::vector<const SamplerVK*>& samplers = pVulkanScene->getSamplers();
	const BufferVK* pMaterialParametersBuffer = pVulkanScene->getMaterialParametersBuffer();

	ImageViewVK* pDepthImageView[1] = { pGBuffer->getDepthImageView() };
	ImageViewVK* pAlbedoImageView[1] = { pGBuffer->getColorAttachment(0) };
	ImageViewVK* pNormalImageView[1] = { pGBuffer->getColorAttachment(1) };

	m_pRayTracingDescriptorSet->writeAccelerationStructureDescriptor(pVulkanScene->getTLAS().AccelerationStructure,									RT_TLAS_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(pAlbedoImageView, &m_pSampler,			1,											RT_GBUFFER_ALBEDO_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(pNormalImageView, &m_pSampler,			1,											RT_GBUFFER_NORMAL_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(pDepthImageView, &m_pSampler,				1,											RT_GBUFFER_DEPTH_BINDING);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(pVulkanScene->getCombinedVertexBuffer(),												RT_COMBINED_VERTEX_BINDING);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(pVulkanScene->getCombinedIndexBuffer(),												RT_COMBINED_INDEX_BINDING);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(pVulkanScene->getMeshIndexBuffer(),													RT_MESH_INDEX_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(albedoMaps.data(), samplers.data(),		MAX_NUM_UNIQUE_MATERIALS,					RT_COMBINED_ALBEDO_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(normalMaps.data(), samplers.data(),		MAX_NUM_UNIQUE_MATERIALS,					RT_COMBINED_NORMAL_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(aoMaps.data(), samplers.data(),			MAX_NUM_UNIQUE_MATERIALS,					RT_COMBINED_AO_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(metallicMaps.data(), samplers.data(),		MAX_NUM_UNIQUE_MATERIALS,					RT_COMBINED_METALLIC_BINDING);
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(roughnessMaps.data(), samplers.data(),	MAX_NUM_UNIQUE_MATERIALS,					RT_COMBINED_ROUGHNESS_BINDING);
	m_pRayTracingDescriptorSet->writeStorageBufferDescriptor(pMaterialParametersBuffer,																RT_COMBINED_MATERIAL_PARAMETERS_BINDING);
	

	vkCmdBindPipeline(m_ppComputeCommandBuffers[currentFrame]->getCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipeline->getPipeline());

	m_ppComputeCommandBuffers[currentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipelineLayout, 0, 1, &m_pRayTracingDescriptorSet, 0, nullptr);
	m_pProfiler->beginTimestamp(&m_TimestampTraceRays);
	m_ppComputeCommandBuffers[currentFrame]->traceRays(m_pRayTracingPipeline->getSBT(), m_RaysWidth, m_RaysHeight, 0);
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

void RayTracingRendererVK::setRayTracingResult(ImageViewVK* pRayTracingResultImageView, uint32_t width, uint32_t height)
{
	m_RaysWidth = width;
	m_RaysHeight = height;
	m_pRayTracingDescriptorSet->writeStorageImageDescriptor(pRayTracingResultImageView, RT_RESULT_IMAGE_BINDING);
}

void RayTracingRendererVK::setSkybox(TextureCubeVK* pSkybox)
{
	m_pSkybox = pSkybox;

	ImageViewVK* pSkyboxView = m_pSkybox->getImageView();
	m_pRayTracingDescriptorSet->writeCombinedImageDescriptors(&pSkyboxView, &m_pSampler, 1, RT_SKYBOX_BINDING);
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
	m_pRayTracingDescriptorSetLayout->addBindingStorageImage(VK_SHADER_STAGE_RAYGEN_BIT_NV,																		RT_RESULT_IMAGE_BINDING,					1);

	//Uniform Buffer
	m_pRayTracingDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_MISS_BIT_NV, RT_CAMERA_BUFFER_BINDING,					1);

	//TLAS
	m_pRayTracingDescriptorSetLayout->addBindingAccelerationStructure(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,						RT_TLAS_BINDING,							1);

	//GBuffer
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, nullptr,															RT_GBUFFER_ALBEDO_BINDING,					1);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, nullptr,															RT_GBUFFER_NORMAL_BINDING,					1);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, nullptr,															RT_GBUFFER_DEPTH_BINDING,					1);

	//Scene Mesh Information
	m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,																RT_COMBINED_VERTEX_BINDING,					1);
	m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,																RT_COMBINED_INDEX_BINDING,					1);
	m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,																RT_MESH_INDEX_BINDING,						1);

	//Scene Material Information
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr,														RT_COMBINED_ALBEDO_BINDING,					MAX_NUM_UNIQUE_MATERIALS);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr,														RT_COMBINED_NORMAL_BINDING,					MAX_NUM_UNIQUE_MATERIALS);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr,														RT_COMBINED_AO_BINDING,						MAX_NUM_UNIQUE_MATERIALS);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr,														RT_COMBINED_METALLIC_BINDING,				MAX_NUM_UNIQUE_MATERIALS);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr,														RT_COMBINED_ROUGHNESS_BINDING,				MAX_NUM_UNIQUE_MATERIALS);
	m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,																RT_COMBINED_MATERIAL_PARAMETERS_BINDING,	1);

	//Cubemap
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_MISS_BIT_NV, nullptr,																RT_SKYBOX_BINDING,							1);

	//Light Buffer
	m_pRayTracingDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV,								RT_LIGHT_BUFFER_BINDING,					1);

	m_pRayTracingDescriptorSetLayout->finalize();

	std::vector<const DescriptorSetLayoutVK*> rayTracingDescriptorSetLayouts = { m_pRayTracingDescriptorSetLayout };
	std::vector<VkPushConstantRange> rayTracingPushConstantRanges;

	//Descriptorpool
	DescriptorCounts descriptorCounts = {};
	descriptorCounts.m_SampledImages = MAX_NUM_UNIQUE_MATERIALS * 6;
	descriptorCounts.m_StorageBuffers = 16;
	descriptorCounts.m_UniformBuffers = 16;
	descriptorCounts.m_StorageImages = 1;
	descriptorCounts.m_AccelerationStructures = 1;

	m_pRayTracingDescriptorPool = new DescriptorPoolVK(m_pContext->getDevice());
	m_pRayTracingDescriptorPool->init(descriptorCounts, 256);
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
		m_pClosestHitShader->setSpecializationConstant<uint32_t>(0, MAX_RECURSIONS);
		m_pClosestHitShader->setSpecializationConstant<uint32_t>(1, MAX_NUM_UNIQUE_MATERIALS);
		hitGroupParams.pClosestHitShader = m_pClosestHitShader;

		m_pClosestHitShadowShader = reinterpret_cast<ShaderVK*>(m_pContext->createShader());
		m_pClosestHitShadowShader->initFromFile(EShader::CLOSEST_HIT_SHADER, "main", "assets/shaders/raytracing/closesthitShadow.spv");
		m_pClosestHitShadowShader->finalize();
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