#include "MeshRendererVK.h"

#include "BufferVK.h"
#include "CommandBufferVK.h"
#include "CommandPoolVK.h"
#include "DescriptorPoolVK.h"
#include "DescriptorSetVK.h"
#include "FrameBufferVK.h"
#include "GraphicsContextVK.h"
#include "ImguiVK.h"
#include "MeshVK.h"
#include "PipelineLayoutVK.h"
#include "PipelineVK.h"
#include "RenderingHandlerVK.h"
#include "RenderPassVK.h"
#include "SamplerVK.h"
#include "SwapChainVK.h"
#include "Texture2DVK.h"

#include "Core/Camera.h"

#include <glm/gtc/type_ptr.hpp>

//TEMP
#include "Ray Tracing/ShaderBindingTableVK.h"
#include "Ray Tracing/RayTracingPipelineVK.h"
#include "Ray Tracing/RayTracingSceneVK.h"
#include "ShaderVK.h"
#include "ImageVK.h"
#include "ImageViewVK.h"

MeshRendererVK::MeshRendererVK(GraphicsContextVK* pContext, RenderingHandlerVK* pRenderingHandler)
	: m_pContext(pContext),
	m_pRenderingHandler(pRenderingHandler),
	m_ppCommandPools(),
	m_ppCommandBuffers(),
	m_pRenderPass(nullptr),
	m_ppBackbuffers(),
	m_pPipeline(nullptr),
	m_pDescriptorSet(nullptr),
	m_pDescriptorPool(nullptr),
	m_pDescriptorSetLayout(nullptr),
	m_Viewport(),
	m_ScissorRect(),
	m_CurrentFrame(0),
	m_BackBufferIndex(0),
	m_pProfiler(nullptr),
	m_pRayTracingScene(nullptr),
	m_pRayTracingPipeline(nullptr),
	m_pRayTracingPipelineLayout(nullptr),
	m_pSBT(nullptr),
	m_pRayTracingStorageImage(nullptr),
	m_pRayTracingStorageImageView(nullptr),
	m_pRayTracingDescriptorPool(nullptr),
	m_pRayTracingDescriptorSetLayout(nullptr),
	m_pRayTracingUniformBuffer(nullptr),
	m_pRaygenShader(nullptr),
	m_pClosestHitShader(nullptr),
	m_pMissShader(nullptr),
	m_pSampler(nullptr)
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_ppComputeCommandPools[i] = nullptr;
	}
}

MeshRendererVK::~MeshRendererVK()
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		SAFEDELETE(m_ppCommandPools[i]);
		if (m_ImageAvailableSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(m_pContext->getDevice()->getDevice(), m_ImageAvailableSemaphores[i], nullptr);
		}

		if (m_RenderFinishedSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(m_pContext->getDevice()->getDevice(), m_RenderFinishedSemaphores[i], nullptr);
		}
	}

	SAFEDELETE(m_pPipelineLayout);
	SAFEDELETE(m_pPipeline);
	SAFEDELETE(m_pDescriptorPool);
	SAFEDELETE(m_pDescriptorSetLayout);

	//Ray Tracing Stuff
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		SAFEDELETE(m_ppComputeCommandPools[i]);
	}

	SAFEDELETE(m_pRayTracingScene);
	SAFEDELETE(m_pRayTracingPipeline);
	SAFEDELETE(m_pRayTracingPipelineLayout);
	SAFEDELETE(m_pSBT);
	SAFEDELETE(m_pRayTracingStorageImage);
	SAFEDELETE(m_pRayTracingStorageImageView);

	SAFEDELETE(m_pRayTracingDescriptorPool);
	SAFEDELETE(m_pRayTracingDescriptorSetLayout);

	SAFEDELETE(m_pRayTracingUniformBuffer);

	SAFEDELETE(m_pRaygenShader);
	SAFEDELETE(m_pClosestHitShader);
	SAFEDELETE(m_pMissShader);

	SAFEDELETE(m_pSampler);

	SAFEDELETE(m_pProfiler);

	m_pContext = nullptr;
}

bool MeshRendererVK::init()
{
	m_pRenderPass = m_pRenderingHandler->getRenderPass();

	// Get and set backbuffers
	FrameBufferVK** ppFrameBuffers = m_pRenderingHandler->getBackBuffers();
	for (size_t frameIdx = 0; frameIdx < MAX_FRAMES_IN_FLIGHT; frameIdx++) {
		m_ppBackbuffers[frameIdx] = ppFrameBuffers[frameIdx];
	}

	if (!createCommandPoolAndBuffers()) {
		return false;
	}

	if (!createPipelineLayouts()) {
		return false;
	}

	if (!createPipelines()) {
		return false;
	}

	if (!createSemaphores()) {
		return false;
	}

	createProfiler();

	// Last thing is to write the camera buffer to the descriptor set
	BufferVK* pCameraBuffer = m_pRenderingHandler->getCameraMatricesBuffer();
	m_pDescriptorSet->writeUniformBufferDescriptor(pCameraBuffer->getBuffer(), 0);

	return true;
}

void MeshRendererVK::beginFrame(const Camera& camera)
{
	//Prepare for frame
	uint32_t backBufferIndex = m_pContext->getSwapChain()->getImageIndex();

	m_ppCommandBuffers[m_CurrentFrame]->reset(false);
	m_ppCommandPools[m_CurrentFrame]->reset();

	// Needed to begin a secondary buffer
	VkCommandBufferInheritanceInfo inheritanceInfo = {};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.pNext = nullptr;
	inheritanceInfo.renderPass = m_pRenderPass->getRenderPass();
	inheritanceInfo.subpass = 0; // TODO: Don't hardcode this :(
	inheritanceInfo.framebuffer = m_ppBackbuffers[backBufferIndex]->getFrameBuffer();

	m_ppCommandBuffers[m_CurrentFrame]->begin(&inheritanceInfo);
	m_pProfiler->beginFrame(m_CurrentFrame, m_ppCommandBuffers[m_CurrentFrame], m_pRenderingHandler->getCurrentGraphicsCommandBuffer());

	m_ppCommandBuffers[m_CurrentFrame]->setViewports(&m_Viewport, 1);
	m_ppCommandBuffers[m_CurrentFrame]->setScissorRects(&m_ScissorRect, 1);

	m_ppCommandBuffers[m_CurrentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &m_pDescriptorSet, 0, nullptr);
}

void MeshRendererVK::endFrame()
{
	m_pProfiler->endFrame();
	m_ppCommandBuffers[m_CurrentFrame]->end();

	DeviceVK* pDevice = m_pContext->getDevice();
	pDevice->executeSecondaryCommandBuffer(m_pRenderingHandler->getCurrentGraphicsCommandBuffer(), m_ppCommandBuffers[m_CurrentFrame]);

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void MeshRendererVK::endRayTraceFrame()
{
	m_ppComputeCommandBuffers[m_CurrentFrame]->end();

	DeviceVK* pDevice = m_pContext->getDevice();
	pDevice->executePrimaryCommandBuffer(pDevice->getComputeQueue(), m_ppComputeCommandBuffers[m_CurrentFrame], nullptr, nullptr, 0, nullptr, 0);

	m_pContext->getDevice()->wait();

	//Prepare for frame
	m_pContext->getSwapChain()->acquireNextImage(m_ImageAvailableSemaphores[m_CurrentFrame]);

	m_ppCommandBuffers[m_CurrentFrame]->reset();
	m_ppCommandPools[m_CurrentFrame]->reset();

	m_ppCommandBuffers[m_CurrentFrame]->begin();

	ImageVK* pCurrentImage = m_pContext->getSwapChain()->getImage(m_pContext->getSwapChain()->getImageIndex());
	m_ppCommandBuffers[m_CurrentFrame]->transitionImageLayout(pCurrentImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_ppCommandBuffers[m_CurrentFrame]->transitionImageLayout(m_pRayTracingStorageImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	VkImageCopy copyRegion{};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = { 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = { 0, 0, 0 };
	copyRegion.extent = { m_pContext->getSwapChain()->getExtent().width, m_pContext->getSwapChain()->getExtent().height, 1 };
	vkCmdCopyImage(m_ppCommandBuffers[m_CurrentFrame]->getCommandBuffer(), m_pRayTracingStorageImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pCurrentImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	m_ppCommandBuffers[m_CurrentFrame]->transitionImageLayout(pCurrentImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	m_ppCommandBuffers[m_CurrentFrame]->transitionImageLayout(m_pRayTracingStorageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

	m_ppCommandBuffers[m_CurrentFrame]->end();

	//Execute commandbuffer
	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	pDevice->executePrimaryCommandBuffer(pDevice->getGraphicsQueue(), m_ppCommandBuffers[m_CurrentFrame], waitSemaphores, waitStages, 1, signalSemaphores, 1);
}

void MeshRendererVK::traceRays()
{
	vkCmdBindPipeline(m_ppComputeCommandBuffers[m_CurrentFrame]->getCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipeline->getPipeline());

	m_ppComputeCommandBuffers[m_CurrentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_pRayTracingPipelineLayout, 0, 1, &m_pRayTracingDescriptorSet, 0, nullptr);

	m_ppComputeCommandBuffers[m_CurrentFrame]->traceRays(m_pSBT, m_pContext->getSwapChain()->getExtent().width, m_pContext->getSwapChain()->getExtent().height, 0);
}

void MeshRendererVK::setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY)
{
	m_Viewport.x = topX;
	m_Viewport.y = topY;
	m_Viewport.width	= width;
	m_Viewport.height	= height;
	m_Viewport.minDepth = minDepth;
	m_Viewport.maxDepth = maxDepth;

	m_ScissorRect.extent.width = width;
	m_ScissorRect.extent.height = height;
	m_ScissorRect.offset.x = 0;
	m_ScissorRect.offset.y = 0;
}

void MeshRendererVK::submitMesh(IMesh* pMesh, const glm::vec4& color, const glm::mat4& transform)
{
	ASSERT(pMesh != nullptr);

	m_ppCommandBuffers[m_CurrentFrame]->bindPipeline(m_pPipeline);

	m_ppCommandBuffers[m_CurrentFrame]->pushConstants(m_pPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,				   sizeof(glm::mat4), (const void*)glm::value_ptr(transform));
	m_ppCommandBuffers[m_CurrentFrame]->pushConstants(m_pPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), (const void*)glm::value_ptr(color));

	BufferVK* pIndexBuffer = reinterpret_cast<BufferVK*>(pMesh->getIndexBuffer());
	m_ppCommandBuffers[m_CurrentFrame]->bindIndexBuffer(pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

    static bool submit = true;
	if (submit)
	{
		BufferVK* pVertBuffer = reinterpret_cast<BufferVK*>(pMesh->getVertexBuffer());
		m_pDescriptorSet->writeStorageBufferDescriptor(pVertBuffer->getBuffer(), 1);

		submit = false;
	}
	m_ppCommandBuffers[m_CurrentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &m_pDescriptorSet, 0, nullptr);

	m_pProfiler->writeTimestamp(&m_TimestampDrawIndexed);
	m_ppCommandBuffers[m_CurrentFrame]->drawIndexInstanced(pMesh->getIndexCount(), 1, 0, 0, 0);
	m_pProfiler->writeTimestamp(&m_TimestampDrawIndexed);
}

bool MeshRendererVK::createSemaphores()
{
	//Create semaphores
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(m_pContext->getDevice()->getDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]), "Failed to create semaphores for a Frame");
		VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(m_pContext->getDevice()->getDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]), "Failed to create semaphores for a Frame");
	}

	return true;
}

bool MeshRendererVK::createCommandPoolAndBuffers()
{
	DeviceVK* pDevice = m_pContext->getDevice();

	const uint32_t queueFamilyIndex = pDevice->getQueueFamilyIndices().graphicsFamily.value();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, queueFamilyIndex);

		if (!m_ppCommandPools[i]->init()) {
			return false;
		}

		m_ppCommandBuffers[i] = m_ppCommandPools[i]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
		if (m_ppCommandBuffers[i] == nullptr) {
			return false;
		}
	}

	return true;
}

bool MeshRendererVK::createPipelines()
{
	//Create pipelinestate
	IShader* pVertexShader = m_pContext->createShader();
	pVertexShader->initFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/vertex.spv");
	if (!pVertexShader->finalize())
	{
		return false;
	}

	IShader* pPixelShader = m_pContext->createShader();
	pPixelShader->initFromFile(EShader::PIXEL_SHADER, "main", "assets/shaders/fragment.spv");
	if (!pPixelShader->finalize())
	{
		return false;
	}

	std::vector<IShader*> shaders = { pVertexShader, pPixelShader };
		m_pPipeline = DBG_NEW PipelineVK(m_pContext->getDevice());
		m_pPipeline->addColorBlendAttachment(false, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
		m_pPipeline->setCulling(true);
		m_pPipeline->setDepthTest(true);
		m_pPipeline->setWireFrame(false);
		//TODO: Return bool
		m_pPipeline->finalizeGraphics(shaders, m_pRenderPass, m_pPipelineLayout);

	SAFEDELETE(pVertexShader);
	SAFEDELETE(pPixelShader);

	return true;
}

bool MeshRendererVK::createPipelineLayouts()
{
	//DescriptorSetLayout
	m_pDescriptorSetLayout = DBG_NEW DescriptorSetLayoutVK(m_pContext->getDevice());
	//CameraBuffer
	m_pDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 0, 1);
	//VertexBuffer
	m_pDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT, 1, 1);
	m_pDescriptorSetLayout->finalize();
	std::vector<const DescriptorSetLayoutVK*> descriptorSetLayouts = { m_pDescriptorSetLayout };

	//Descriptorpool
	DescriptorCounts descriptorCounts = {};
	descriptorCounts.m_SampledImages	= 128;
	descriptorCounts.m_StorageBuffers	= 128;
	descriptorCounts.m_UniformBuffers	= 128;

	m_pDescriptorPool = DBG_NEW DescriptorPoolVK(m_pContext->getDevice());
	m_pDescriptorPool->init(descriptorCounts, 16);
	m_pDescriptorSet = m_pDescriptorPool->allocDescriptorSet(m_pDescriptorSetLayout);
	if (m_pDescriptorSet == nullptr)
	{
		return false;
	}

	//PushConstant - Triangle color
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.size = sizeof(glm::mat4) + sizeof(glm::vec4);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	std::vector<VkPushConstantRange> pushConstantRanges = { pushConstantRange };

	m_pPipelineLayout = DBG_NEW PipelineLayoutVK(m_pContext->getDevice());
	
	//TODO: Return bool
	m_pPipelineLayout->init(descriptorSetLayouts, pushConstantRanges);

	return true;
}

bool MeshRendererVK::createRayTracingPipelineLayouts()
{
	m_pRayTracingDescriptorSetLayout = new DescriptorSetLayoutVK(m_pContext->getDevice());
	m_pRayTracingDescriptorSetLayout->addBindingAccelerationStructure(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 0, 1);
	m_pRayTracingDescriptorSetLayout->addBindingStorageImage(VK_SHADER_STAGE_RAYGEN_BIT_NV, 1, 1);
	m_pRayTracingDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV | VK_SHADER_STAGE_MISS_BIT_NV, 2, 1);
	m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 3, 1);
	m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 4, 1);
	m_pRayTracingDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 5, 1);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr, 6, MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr, 7, MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
	m_pRayTracingDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, nullptr, 8, MAX_NUM_UNIQUE_GRAPHICS_OBJECT_TEXTURES);
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

void MeshRendererVK::createProfiler()
{
	m_pProfiler = DBG_NEW ProfilerVK("Mesh Renderer", m_pContext->getDevice());

	m_pProfiler->initTimestamp(&m_TimestampDrawIndexed, "Draw indexed");
}
