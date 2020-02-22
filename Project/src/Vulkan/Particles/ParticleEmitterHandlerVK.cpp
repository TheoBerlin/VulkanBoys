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
	m_pSampler(nullptr),
	m_CurrentFrame(0)
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
    }

	SAFEDELETE(m_pDescriptorPool);
	SAFEDELETE(m_pDescriptorSetLayout);
	SAFEDELETE(m_pPipelineLayout);
	SAFEDELETE(m_pPipeline);
	SAFEDELETE(m_pSampler);
}

void ParticleEmitterHandlerVK::update(float dt)
{
	if (m_GPUComputed) {
        updateGPU(dt);
    } else {
        for (ParticleEmitter* particleEmitter : m_ParticleEmitters) {
            particleEmitter->update(dt);
        }
    }
}

void ParticleEmitterHandlerVK::updateRenderingBuffers(IRenderingHandler* pRenderingHandler)
{
    RenderingHandlerVK* pRenderingHandlerVK = reinterpret_cast<RenderingHandlerVK*>(pRenderingHandler);
    CommandBufferVK* pCommandBuffer = pRenderingHandlerVK->getCurrentCommandBuffer();

    for (ParticleEmitter* pEmitter : m_ParticleEmitters) {
        BufferVK* pPositionsBuffer = reinterpret_cast<BufferVK*>(pEmitter->getPositionsBuffer());

		if (m_GPUComputed) {
			// Transition particle positions buffer
			prepBufferForRendering(pPositionsBuffer, pCommandBuffer);
		} else {
			// Update emitter buffer. If GPU computing is enabled, this will already have been updated
			if (pEmitter->m_EmitterUpdated) {
				EmitterBuffer emitterBuffer = {};
				pEmitter->createEmitterBuffer(emitterBuffer);

				BufferVK* pEmitterBuffer = reinterpret_cast<BufferVK*>(pEmitter->getEmitterBuffer());
				pCommandBuffer->updateBuffer(pEmitterBuffer, 0, &emitterBuffer, sizeof(EmitterBuffer));

				pEmitter->m_EmitterUpdated = false;
			}

			// Update particle positions buffer
			const std::vector<glm::vec4>& particlePositions = pEmitter->getParticleStorage().positions;

			pCommandBuffer->updateBuffer(pPositionsBuffer, 0, particlePositions.data(), sizeof(glm::vec4) * particlePositions.size());
		}
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

void ParticleEmitterHandlerVK::initializeEmitter(ParticleEmitter* pEmitter)
{
	pEmitter->initialize(m_pGraphicsContext, m_pCamera);
/*
	// A temporary command buffer is needed as it is not known if one of the member buffers have begun recording or not
	CommandBufferVK* pTempCommandBuffer = m_ppCommandPools[0]->allocateCommandBuffer();
	pTempCommandBuffer->reset();
	pTempCommandBuffer->begin();

	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
	DeviceVK* pDevice = pGraphicsContext->getDevice();
	const QueueFamilyIndices& queueFamilyIndices = pDevice->getQueueFamilyIndices();

	VkBufferMemoryBarrier bufferMemoryBarrier = {};
	bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemoryBarrier.pNext = nullptr;
	bufferMemoryBarrier.srcAccessMask = 0;
	bufferMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	//bufferMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	// It is unknown if the rendering or computing is next
	bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	// bufferMemoryBarrier.srcQueueFamilyIndex = queueFamilyIndices.computeFamily.value();
	// bufferMemoryBarrier.dstQueueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	bufferMemoryBarrier.offset = 0;
	bufferMemoryBarrier.size = VK_WHOLE_SIZE;

	std::vector<BufferVK*> emitterBuffers = {
		//reinterpret_cast<BufferVK*>(pEmitter->getPositionsBuffer()),
		reinterpret_cast<BufferVK*>(pEmitter->getVelocitiesBuffer()),
		reinterpret_cast<BufferVK*>(pEmitter->getAgesBuffer())
	};

	// The positions buffer will be used for both rendering and computing and it is unknown if
	// the particles will be used for computing or rendering next, so leave the door open for either option.
	VkPipelineStageFlags dstFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	for (BufferVK* pBuffer : emitterBuffers) {
		bufferMemoryBarrier.buffer = pBuffer->getBuffer();

		vkCmdPipelineBarrier(
			pTempCommandBuffer->getCommandBuffer(),
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			dstFlags,
			0,
			0, nullptr,
			1, &bufferMemoryBarrier,
			0, nullptr);

		// The velocities and ages buffers are only used for computing
		//dstFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}

	// TODO: If the renderer is next to use the buffers, it will use the graphics queue. There is no guarantee that this transition will finish by then.
	pDevice->executePrimaryCommandBuffer(pDevice->getComputeQueue(), pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
	m_ppCommandPools[0]->freeCommandBuffer(&pTempCommandBuffer);*/
}

void ParticleEmitterHandlerVK::updateGPU(float dt)
{
	beginUpdateFrame();

	for (ParticleEmitter* pEmitter : m_ParticleEmitters) {
		pEmitter->updateGPU(dt);

		// Update descriptor set
		BufferVK* pPositionsBuffer = reinterpret_cast<BufferVK*>(pEmitter->getPositionsBuffer());
		BufferVK* pVelocitiesBuffer = reinterpret_cast<BufferVK*>(pEmitter->getVelocitiesBuffer());
		BufferVK* pAgesBuffer = reinterpret_cast<BufferVK*>(pEmitter->getAgesBuffer());
		BufferVK* pEmitterBuffer = reinterpret_cast<BufferVK*>(pEmitter->getEmitterBuffer());

		//prepBufferForCompute(pPositionsBuffer);
		// TODO: The two transitions beneath are not needed, are they...?
		//prepBufferForCompute(pVelocitiesBuffer);
		//prepBufferForCompute(pAgesBuffer);

		// TODO: Use constant variables or define macros for binding indices
		m_ppCommandBuffers[m_CurrentFrame]->pushConstants(m_pPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), (const void*)&dt);
		m_ppDescriptorSets[m_CurrentFrame]->writeStorageBufferDescriptor(pPositionsBuffer->getBuffer(), 0);
		m_ppDescriptorSets[m_CurrentFrame]->writeStorageBufferDescriptor(pVelocitiesBuffer->getBuffer(), 1);
		m_ppDescriptorSets[m_CurrentFrame]->writeStorageBufferDescriptor(pAgesBuffer->getBuffer(), 2);
		m_ppDescriptorSets[m_CurrentFrame]->writeUniformBufferDescriptor(pEmitterBuffer->getBuffer(), 3);

		uint32_t particleCount = pEmitter->getParticleCount();
		glm::u32vec3 workGroupSize(particleCount, 0, 0);

		m_ppCommandBuffers[m_CurrentFrame]->dispatch(workGroupSize);
    }

	endUpdateFrame();
}

void ParticleEmitterHandlerVK::prepBufferForCompute(BufferVK* pBuffer)
{
	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
	DeviceVK* pDevice = pGraphicsContext->getDevice();
	const QueueFamilyIndices& queueFamilyIndices = pDevice->getQueueFamilyIndices();

	VkBufferMemoryBarrier bufferMemoryBarrier = {};
	bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemoryBarrier.pNext = nullptr;

	//bufferMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	// bufferMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	// bufferMemoryBarrier.srcQueueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	// bufferMemoryBarrier.dstQueueFamilyIndex = queueFamilyIndices.computeFamily.value();
	bufferMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	bufferMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferMemoryBarrier.buffer = pBuffer->getBuffer();
	bufferMemoryBarrier.offset = 0;
	bufferMemoryBarrier.size = VK_WHOLE_SIZE;

	vkCmdPipelineBarrier(
		m_ppCommandBuffers[m_CurrentFrame]->getCommandBuffer(),
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0,
		0, nullptr,
		1, &bufferMemoryBarrier,
		0, nullptr);
}

void ParticleEmitterHandlerVK::prepBufferForRendering(BufferVK* pBuffer, CommandBufferVK* pCommandBuffer)
{
	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
	DeviceVK* pDevice = pGraphicsContext->getDevice();
	const QueueFamilyIndices& queueFamilyIndices = pDevice->getQueueFamilyIndices();

	VkBufferMemoryBarrier bufferMemoryBarrier = {};
	bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemoryBarrier.pNext = nullptr;
	bufferMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	bufferMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;// | VK_ACCESS_SHADER_WRITE_BIT;
	//bufferMemoryBarrier.srcQueueFamilyIndex = queueFamilyIndices.computeFamily.value();
	bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//bufferMemoryBarrier.dstQueueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	bufferMemoryBarrier.buffer = pBuffer->getBuffer();
	bufferMemoryBarrier.offset = 0;
	bufferMemoryBarrier.size = VK_WHOLE_SIZE;

	vkCmdPipelineBarrier(
		pCommandBuffer->getCommandBuffer(),
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		1, &bufferMemoryBarrier,
		0, nullptr);
}

void ParticleEmitterHandlerVK::beginUpdateFrame()
{
	m_ppCommandBuffers[m_CurrentFrame]->reset(true);
	m_ppCommandPools[m_CurrentFrame]->reset();

	m_ppCommandBuffers[m_CurrentFrame]->begin(nullptr);

	m_ppCommandBuffers[m_CurrentFrame]->bindPipeline(m_pPipeline);
	m_ppCommandBuffers[m_CurrentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, m_pPipelineLayout, 0, 1, &m_ppDescriptorSets[m_CurrentFrame], 0, nullptr);

	// Update emitter buffers
	for (ParticleEmitter* pEmitter : m_ParticleEmitters) {
		if (pEmitter->m_EmitterUpdated) {
			EmitterBuffer emitterBuffer = {};
			pEmitter->createEmitterBuffer(emitterBuffer);

			BufferVK* pEmitterBuffer = reinterpret_cast<BufferVK*>(pEmitter->getEmitterBuffer());
			m_ppCommandBuffers[m_CurrentFrame]->updateBuffer(pEmitterBuffer, 0, &emitterBuffer, sizeof(EmitterBuffer));

			pEmitter->m_EmitterUpdated = false;
		}
	}
}

void ParticleEmitterHandlerVK::endUpdateFrame()
{
	m_ppCommandBuffers[m_CurrentFrame]->end();

	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
    DeviceVK* pDevice = pGraphicsContext->getDevice();

	pDevice->executePrimaryCommandBuffer(pDevice->getComputeQueue(), m_ppCommandBuffers[m_CurrentFrame], nullptr, nullptr, 0, nullptr, 0);

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool ParticleEmitterHandlerVK::createCommandPoolAndBuffers()
{
    GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
    DeviceVK* pDevice = pGraphicsContext->getDevice();

	const uint32_t computeQueueIndex = pDevice->getQueueFamilyIndices().computeFamily.value();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_ppCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, computeQueueIndex);

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

	// Positions
	m_pDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_COMPUTE_BIT, 0, 1);

	// Velocities
	m_pDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_COMPUTE_BIT, 1, 1);

	// Ages
	m_pDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_COMPUTE_BIT, 2, 1);

	// Emitter properties
	m_pDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_COMPUTE_BIT, 3, 1);

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

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.size = sizeof(float);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushConstantRange.offset = 0;

	return m_pPipelineLayout->init(descriptorSetLayouts, {pushConstantRange});
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
