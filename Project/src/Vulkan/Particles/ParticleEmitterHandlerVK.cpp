#include "ParticleEmitterHandlerVK.h"

#include "Common/Debug.h"
#include "Common/IGraphicsContext.h"
#include "Common/IShader.h"
#include "Vulkan/BufferVK.h"
#include "Vulkan/CommandPoolVK.h"
#include "Vulkan/CommandBufferVK.h"
#include "Vulkan/DescriptorPoolVK.h"
#include "Vulkan/DescriptorSetVK.h"
#include "Vulkan/DescriptorSetLayoutVK.h"
#include "Vulkan/GraphicsContextVK.h"
#include "Vulkan/PipelineLayoutVK.h"
#include "Vulkan/PipelineVK.h"
#include "Vulkan/RenderingHandlerVK.h"
#include "Vulkan/SamplerVK.h"

ParticleEmitterHandlerVK::ParticleEmitterHandlerVK()
	:m_pDescriptorPool(nullptr),
	m_pDescriptorSetLayout(nullptr),
	m_pPipelineLayout(nullptr),
	m_pPipeline(nullptr),
	m_pSampler(nullptr)
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_ppCommandPools[i] = nullptr;
		m_ppDescriptorSets[i] = nullptr;
    }
}

ParticleEmitterHandlerVK::~ParticleEmitterHandlerVK()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        SAFEDELETE(m_ppCommandPools[i]);
		SAFEDELETE(m_ppDescriptorSets[i]);
    }

	SAFEDELETE(m_pDescriptorPool);
	SAFEDELETE(m_pDescriptorSetLayout);
	SAFEDELETE(m_pPipelineLayout);
	SAFEDELETE(m_pPipeline);
	SAFEDELETE(m_pSampler);
}

void ParticleEmitterHandlerVK::updateBuffers(IRenderingHandler* pRenderingHandler)
{
    RenderingHandlerVK* pRenderingHandlerVK = reinterpret_cast<RenderingHandlerVK*>(pRenderingHandler);
    CommandBufferVK* pCommandBuffer = pRenderingHandlerVK->getCurrentCommandBuffer();

    for (ParticleEmitter* pEmitter : m_ParticleEmitters) {
        BufferVK* pEmitterBuffer = reinterpret_cast<BufferVK*>(pEmitter->getEmitterBuffer());
        BufferVK* pParticleBuffer = reinterpret_cast<BufferVK*>(pEmitter->getParticleBuffer());

        if (pEmitter->m_EmitterUpdated) {
            EmitterBuffer emitterBuffer = {};
			pEmitter->createEmitterBuffer(emitterBuffer);
            pCommandBuffer->updateBuffer(pEmitterBuffer, 0, &emitterBuffer, sizeof(EmitterBuffer));

            pEmitter->m_EmitterUpdated = false;
        }

        const std::vector<glm::vec4>& particlePositions = pEmitter->getParticleStorage().positions;
        ParticleBuffer particleBuffer = {
            particlePositions.data()
        };

        pCommandBuffer->updateBuffer(pParticleBuffer, 0, particlePositions.data(), sizeof(glm::vec4) * particlePositions.size());
    }
}

bool ParticleEmitterHandlerVK::initializeGPUCompute()
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

	return true;
}

void ParticleEmitterHandlerVK::toggleComputationDevice()
{
    m_GPUComputed = !m_GPUComputed;
}

void ParticleEmitterHandlerVK::updateGPU(float dt)
{
	for (ParticleEmitter* particleEmitter : m_ParticleEmitters) {
        particleEmitter->update(dt);
    }
}

bool ParticleEmitterHandlerVK::createCommandPoolAndBuffers()
{
    GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
    DeviceVK* pDevice = pGraphicsContext->getDevice();

	const uint32_t graphicsQueueIndex = pDevice->getQueueFamilyIndices().graphicsFamily.value();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_ppCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, graphicsQueueIndex);

		if (!m_ppCommandPools[i]->init()) {
			return false;
		}

		m_ppCommandBuffers[i] = m_ppCommandPools[i]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		if (m_ppCommandBuffers[i] == nullptr) {
			return false;
		}
	}

	return true;
}

bool ParticleEmitterHandlerVK::createPipelineLayout()
{
	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
    DeviceVK* pDevice = pGraphicsContext->getDevice();

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

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_ppDescriptorSets[i] = m_pDescriptorPool->allocDescriptorSet(m_pDescriptorSetLayout);
		if (m_ppDescriptorSets[i] == nullptr) {
			LOG("Failed to create descriptor set for particle renderer");
			return false;
		}
	}

	m_pPipelineLayout = DBG_NEW PipelineLayoutVK(pDevice);
	return m_pPipelineLayout->init(descriptorSetLayouts, {});
}

bool ParticleEmitterHandlerVK::createPipeline()
{
	// Create pipeline state
	IShader* pComputeShader = m_pGraphicsContext->createShader();
	pComputeShader->initFromFile(EShader::COMPUTE_SHADER, "main", "assets/shaders/particles/update_cs.spv");
	if (!pComputeShader->finalize()) {
        LOG("Failed to create compute shader for particle emitter handler");
		return false;
	}

	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
    DeviceVK* pDevice = pGraphicsContext->getDevice();

	m_pPipeline = DBG_NEW PipelineVK(pDevice);
	m_pPipeline->finalizeCompute(pComputeShader, m_pPipelineLayout);

	SAFEDELETE(pComputeShader);
	return true;
}

void ParticleEmitterHandlerVK::writeBufferDescriptors()
{
	// // Storage buffer for quad vertices
	// BufferVK* pVertBuffer = reinterpret_cast<BufferVK*>(m_pQuadMesh->getVertexBuffer());

	// // Camera buffers
	// BufferVK* pCameraMatricesBuffer = m_pRenderingHandler->getCameraMatricesBuffer();
	// BufferVK* pCameraDirectionsBuffer = m_pRenderingHandler->getCameraDirectionsBuffer();

	// for (DescriptorSetVK* pDescriptorSet : m_ppDescriptorSets) {
	// 	pDescriptorSet->writeStorageBufferDescriptor(pVertBuffer->getBuffer(), 0);

	// 	pDescriptorSet->writeUniformBufferDescriptor(pCameraMatricesBuffer->getBuffer(), 1);
	// 	pDescriptorSet->writeUniformBufferDescriptor(pCameraDirectionsBuffer->getBuffer(), 2);
	// }
}
