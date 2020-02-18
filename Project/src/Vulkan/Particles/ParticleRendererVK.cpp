#include "ParticleRendererVK.h"

#include "Common/IShader.h"
#include "Core/ParticleEmitter.h"
#include "Vulkan/BufferVK.h"
#include "Vulkan/DescriptorPoolVK.h"
#include "Vulkan/DescriptorSetVK.h"
#include "Vulkan/DescriptorSetLayoutVK.h"
#include "Vulkan/CommandBufferVK.h"
#include "Vulkan/CommandPoolVK.h"
#include "Vulkan/FrameBufferVK.h"
#include "Vulkan/GraphicsContextVK.h"
#include "Vulkan/MeshVK.h"
#include "Vulkan/PipelineLayoutVK.h"
#include "Vulkan/PipelineVK.h"
#include "Vulkan/RenderingHandlerVK.h"
#include "Vulkan/RenderPassVK.h"
#include "Vulkan/SamplerVK.h"

#include <array>

ParticleRendererVK::ParticleRendererVK(GraphicsContextVK* pGraphicsContext, RenderingHandlerVK* pRenderingHandler)
	:m_pGraphicsContext(pGraphicsContext),
	m_pRenderingHandler(pRenderingHandler),
	m_pDescriptorPool(nullptr),
	m_pDescriptorSetLayout(nullptr),
	m_pDescriptorSet(nullptr),
	m_pPipelineLayout(nullptr),
	m_pPipeline(nullptr),
	m_pQuadMesh(nullptr),
	m_pSampler(nullptr)
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_ppCommandPools[i] = nullptr;
		m_ppCommandBuffers[i] = nullptr;
	}
}

ParticleRendererVK::~ParticleRendererVK()
{
	for (CommandPoolVK* pCommandPool : m_ppCommandPools) {
		SAFEDELETE(pCommandPool);
	}

	SAFEDELETE(m_pPipelineLayout);
	SAFEDELETE(m_pPipeline);
	SAFEDELETE(m_pQuadMesh);
	SAFEDELETE(m_pSampler);
}

bool ParticleRendererVK::init()
{
	if (!createCommandPoolAndBuffers()) {
		return false;
	}

	if (!createPipelineLayout()) {
		return false;
	}

	if (!createPipeline()) {
		return false;
	}

	if (!createQuadMesh()) {
		return false;
	}

	writeBufferDescriptors();
	return true;
}

void ParticleRendererVK::beginFrame(const Camera& camera)
{
	// Prepare for frame
	uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();

	m_ppCommandBuffers[frameIndex]->reset(false);
	m_ppCommandPools[frameIndex]->reset();

	// Needed to begin a secondary buffer
	VkCommandBufferInheritanceInfo inheritanceInfo = {};
	inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.pNext = nullptr;
	inheritanceInfo.renderPass = m_pRenderingHandler->getRenderPass()->getRenderPass();
	inheritanceInfo.subpass = 0; // TODO: Don't hardcode this :(
	inheritanceInfo.framebuffer = m_pRenderingHandler->getCurrentBackBuffer()->getFrameBuffer();

	m_ppCommandBuffers[frameIndex]->begin(&inheritanceInfo);

	m_ppCommandBuffers[frameIndex]->setViewports(&m_Viewport, 1);
	m_ppCommandBuffers[frameIndex]->setScissorRects(&m_ScissorRect, 1);

	// Bind quad
	BufferVK* pIndexBuffer = reinterpret_cast<BufferVK*>(m_pQuadMesh->getIndexBuffer());
	m_ppCommandBuffers[frameIndex]->bindIndexBuffer(pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	m_ppCommandBuffers[frameIndex]->bindGraphicsPipeline(m_pPipeline);
	//m_ppCommandBuffers[frameIndex]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &m_pDescriptorSet, 0, nullptr);
}

void ParticleRendererVK::endFrame()
{
	uint32_t currentFrame = m_pRenderingHandler->getCurrentFrameIndex();
	m_ppCommandBuffers[currentFrame]->end();

	DeviceVK* pDevice = m_pGraphicsContext->getDevice();
	pDevice->executeSecondaryCommandBuffer(m_pRenderingHandler->getCurrentCommandBuffer(), m_ppCommandBuffers[currentFrame]);
}

void ParticleRendererVK::submitParticles(ParticleEmitter* pEmitter)
{
	uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();

	BufferVK* pEmitterBuffer = reinterpret_cast<BufferVK*>(pEmitter->getEmitterBuffer());
	BufferVK* pParticleBuffer = reinterpret_cast<BufferVK*>(pEmitter->getParticleBuffer());

	m_pDescriptorSet->writeUniformBufferDescriptor(pEmitterBuffer->getBuffer(), 3);
	m_pDescriptorSet->writeStorageBufferDescriptor(pParticleBuffer->getBuffer(), 4);

	// TODO: When does the descriptor set need to be bound? Mesh renderer binds it twice...
	m_ppCommandBuffers[frameIndex]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &m_pDescriptorSet, 0, nullptr);

	uint32_t particleCount = pEmitter->getParticleCount();
	m_ppCommandBuffers[frameIndex]->drawIndexInstanced(m_pQuadMesh->getIndexCount(), particleCount, 0, 0, 0);
}

void ParticleRendererVK::setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY)
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

bool ParticleRendererVK::createCommandPoolAndBuffers()
{
	DeviceVK* pDevice = m_pGraphicsContext->getDevice();

	const uint32_t graphicsQueueIndex = pDevice->getQueueFamilyIndices().graphicsFamily.value();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_ppCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, graphicsQueueIndex);

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

bool ParticleRendererVK::createPipelineLayout()
{
	DeviceVK* pDevice = m_pGraphicsContext->getDevice();

	// Descriptor Set Layout
	m_pDescriptorSetLayout = DBG_NEW DescriptorSetLayoutVK(pDevice);

	// Vertex Buffer
	m_pDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT, 0, 1);

	// Transform matrices Buffer
	m_pDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 1, 1);

	// Camera vectors Buffer
	m_pDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 2, 1);

	// Per-emitter Buffer
	m_pDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 3, 1);

	// Per-particle Buffer
	m_pDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT, 4, 1);

	// Particle Texture
	m_pSampler = new SamplerVK(pDevice);
	m_pSampler->init(VkFilter::VK_FILTER_LINEAR, VkFilter::VK_FILTER_LINEAR, VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT, VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT);
	VkSampler sampler = m_pSampler->getSampler();

	m_pDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, &sampler, 5, 1);

	if (!m_pDescriptorSetLayout->finalize()) {
		LOG("Failed to finalize descriptor set layout");
		return false;
	}

	std::vector<const DescriptorSetLayoutVK*> descriptorSetLayouts = { m_pDescriptorSetLayout };

	// Descriptor pool
	DescriptorCounts descriptorCounts	= {};
	descriptorCounts.m_SampledImages	= 128;
	descriptorCounts.m_StorageBuffers	= 128;
	descriptorCounts.m_UniformBuffers	= 128;

	m_pDescriptorPool = DBG_NEW DescriptorPoolVK(pDevice);
	if (!m_pDescriptorPool->init(descriptorCounts, 16)) {
		LOG("Failed to initialize descriptor pool");
		return false;
	}

	m_pDescriptorSet = m_pDescriptorPool->allocDescriptorSet(m_pDescriptorSetLayout);
	if (m_pDescriptorSet == nullptr) {
		LOG("Failed to create descriptor set for particle renderer");
		return false;
	}

	m_pPipelineLayout = DBG_NEW PipelineLayoutVK(pDevice);
	return m_pPipelineLayout->init(descriptorSetLayouts, {});
}

bool ParticleRendererVK::createPipeline()
{
	// Create pipeline state
	IShader* pVertexShader = m_pGraphicsContext->createShader();
	pVertexShader->initFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/particles/vertex.spv");
	if (!pVertexShader->finalize()) {
        LOG("Failed to create vertex shader for particle emitter handler");
		return false;
	}

	IShader* pPixelShader = m_pGraphicsContext->createShader();
	pPixelShader->initFromFile(EShader::PIXEL_SHADER, "main", "assets/shaders/particles/fragment.spv");
	if (!pPixelShader->finalize()) {
		LOG("Failed to create pixel shader for particle emitter handler");
		return false;
	}

	std::vector<IShader*> shaders = { pVertexShader, pPixelShader };
	m_pPipeline = DBG_NEW PipelineVK(m_pGraphicsContext->getDevice());
	m_pPipeline->addColorBlendAttachment(true, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
	m_pPipeline->setCulling(false);
	m_pPipeline->setDepthTest(false);
	m_pPipeline->setWireFrame(false);
	m_pPipeline->finalize(shaders, m_pRenderingHandler->getRenderPass(), m_pPipelineLayout);

	SAFEDELETE(pVertexShader);
	SAFEDELETE(pPixelShader);

	return true;
}

bool ParticleRendererVK::createQuadMesh()
{
	const std::array<QuadVertex, 4> pQuadVertices{{
		{
			{0.0f, 1.0f}, {0.0f, 1.0f}
		},
		{
			{0.0f, 0.0f}, {0.0f, 0.0f}
		},
		{
			{1.0f, 0.0f}, {1.0f, 0.0f}
		},
		{
			{1.0f, 1.0f}, {1.0f, 1.0f}
		}
	}};

	const std::array<uint32_t, 6> pQuadIndices = {0, 1, 2, 2, 3, 0};

	m_pQuadMesh = DBG_NEW MeshVK(m_pGraphicsContext->getDevice());
	if (!m_pQuadMesh->initFromMemory(pQuadVertices.data(), sizeof(QuadVertex), pQuadVertices.size(), pQuadIndices.data(), pQuadIndices.size())) {
		return false;
	}

	// Write descriptor and bind quad
	BufferVK* pVertBuffer = reinterpret_cast<BufferVK*>(m_pQuadMesh->getVertexBuffer());
	VkDeviceSize offsets = 0;

	m_pDescriptorSet->writeStorageBufferDescriptor(pVertBuffer->getBuffer(), 0);

	return true;
}

void ParticleRendererVK::writeBufferDescriptors()
{
	// Storage buffer for quad vertices
	BufferVK* pVertBuffer = reinterpret_cast<BufferVK*>(m_pQuadMesh->getVertexBuffer());
	m_pDescriptorSet->writeStorageBufferDescriptor(pVertBuffer->getBuffer(), 0);

	// Camera buffers
	BufferVK* pCameraMatricesBuffer = m_pRenderingHandler->getCameraMatricesBuffer();
	m_pDescriptorSet->writeUniformBufferDescriptor(pCameraMatricesBuffer->getBuffer(), 1);

	BufferVK* pCameraDirectionsBuffer = m_pRenderingHandler->getCameraDirectionsBuffer();
	m_pDescriptorSet->writeUniformBufferDescriptor(pCameraDirectionsBuffer->getBuffer(), 2);
}
