#include "ParticleRendererVK.h"

#include "Common/IShader.h"
#include "Core/ParticleEmitter.hpp"
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

#include <array>

ParticleRendererVK::ParticleRendererVK(GraphicsContextVK* pGraphicsContext, RenderingHandlerVK* pRenderingHandler)
	:m_pGraphicsContext(pGraphicsContext),
	m_pRenderingHandler(pRenderingHandler),
	m_pRenderPass(nullptr),
	m_pDescriptorPool(nullptr),
	m_pDescriptorSetLayout(nullptr),
	m_pDescriptorSet(nullptr),
	m_pPipelineLayout(nullptr),
	m_pPipeline(nullptr),
	m_pQuadMesh(nullptr)
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

	SAFEDELETE(m_pRenderPass);
	SAFEDELETE(m_pPipelineLayout);
	SAFEDELETE(m_pPipeline);
	SAFEDELETE(m_pQuadMesh);
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

	m_ppCommandBuffers[frameIndex]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &m_pDescriptorSet, 0, nullptr);
}

void ParticleRendererVK::endFrame()
{

}

void ParticleRendererVK::submitParticles(const ParticleEmitter* pParticleEmitter)
{

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
	// Descriptor Set Layout
	m_pDescriptorSetLayout = DBG_NEW DescriptorSetLayoutVK(m_pGraphicsContext->getDevice());

	// Vertex Buffer
	m_pDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT, 0, 1);

	// Transform matrices Buffer
	m_pDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 1, 1);

	// Camera vectors Buffer
	m_pDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 2, 1);

	// Per-particle Buffer
	m_pDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 3, 1);

	// Per-emitter Buffer
	m_pDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 4, 1);

	if (!m_pDescriptorSetLayout->finalize()) {
		return false;
	}

	std::vector<const DescriptorSetLayoutVK*> descriptorSetLayouts = { m_pDescriptorSetLayout };

	// Descriptor pool
	DescriptorCounts descriptorCounts	= {};
	descriptorCounts.m_SampledImages	= 128;
	descriptorCounts.m_StorageBuffers	= 128;
	descriptorCounts.m_UniformBuffers	= 128;

	m_pDescriptorPool = DBG_NEW DescriptorPoolVK(m_pGraphicsContext->getDevice());
	if (m_pDescriptorPool->init(descriptorCounts, 16)) {
		return false;
	}

	m_pDescriptorSet = m_pDescriptorPool->allocDescriptorSet(m_pDescriptorSetLayout);
	if (m_pDescriptorSet == nullptr) {
		return false;
	}

	m_pPipelineLayout = DBG_NEW PipelineLayoutVK(m_pGraphicsContext->getDevice());
	return m_pPipelineLayout->init(descriptorSetLayouts, {});
}

bool ParticleRendererVK::createPipeline()
{
	// Create pipeline state
	IShader* pVertexShader = m_pGraphicsContext->createShader();
	pVertexShader->initFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/particles/vertex.spv");
	if (!pVertexShader->finalize()) {
        LOG("Failed to create vertex shader for particle emitter handler");
		return;
	}

	IShader* pPixelShader = m_pGraphicsContext->createShader();
	pPixelShader->initFromFile(EShader::PIXEL_SHADER, "main", "assets/shaders/particles/fragment.spv");
	if (!pPixelShader->finalize()) {
		LOG("Failed to create pixel shader for particle emitter handler");
		return;
	}

	std::vector<IShader*> shaders = { pVertexShader, pPixelShader };
	m_pPipeline = DBG_NEW PipelineVK(m_pGraphicsContext->getDevice());
	m_pPipeline->addColorBlendAttachment(true, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
	m_pPipeline->setCulling(false);
	m_pPipeline->setDepthTest(false);
	m_pPipeline->setWireFrame(false);
	m_pPipeline->finalize(shaders, m_pRenderPass, m_pPipelineLayout);

	SAFEDELETE(pVertexShader);
	SAFEDELETE(pPixelShader);
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

	return true;
}
