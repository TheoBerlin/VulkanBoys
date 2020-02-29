#include "ParticleEmitterHandlerVK.h"

#include "imgui/imgui.h"

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
#include "Vulkan/ShaderVK.h"

ParticleEmitterHandlerVK::ParticleEmitterHandlerVK()
	:m_pDescriptorPool(nullptr),
	m_pDescriptorSetLayout(nullptr),
	m_pPipelineLayout(nullptr),
	m_pPipeline(nullptr),
	m_pCommandPoolGraphics(nullptr),
	m_WorkGroupSize(0),
	m_CurrentFrame(0),
	m_pProfiler(nullptr)
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
	SAFEDELETE(m_pCommandPoolGraphics);

	SAFEDELETE(m_pProfiler);
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
    CommandBufferVK* pCommandBuffer = pRenderingHandlerVK->getCurrentGraphicsCommandBuffer();

    for (ParticleEmitter* pEmitter : m_ParticleEmitters) {
		if (!m_GPUComputed) {
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
			BufferVK* pPositionsBuffer = reinterpret_cast<BufferVK*>(pEmitter->getPositionsBuffer());

			pCommandBuffer->updateBuffer(pPositionsBuffer, 0, particlePositions.data(), sizeof(glm::vec4) * particlePositions.size());
		}
    }
}

void ParticleEmitterHandlerVK::drawProfileUI()
{
	if (m_GPUComputed) {
		// The root profiler creates the profiler window
        ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
	    if (ImGui::Begin("Profiler", NULL, ImGuiWindowFlags_NoResize)) {
			m_pProfiler->drawResults();
        }

		ImGui::End();
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

	createProfiler();

	return true;
}

void ParticleEmitterHandlerVK::toggleComputationDevice()
{
	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
	DeviceVK* pDevice = pGraphicsContext->getDevice();

	if (m_GPUComputed) {
		// Disable GPU-side computing
		CommandBufferVK* pTempCommandBuffer = m_pCommandPoolGraphics->allocateCommandBuffer();
		pTempCommandBuffer->reset();
		pTempCommandBuffer->begin();

		for (ParticleEmitter* pEmitter : m_ParticleEmitters) {
			BufferVK* pPositionsBuffer = reinterpret_cast<BufferVK*>(pEmitter->getPositionsBuffer());
			acquireForGraphics(pPositionsBuffer, pTempCommandBuffer);
		}

		pTempCommandBuffer->end();
		pDevice->executePrimaryCommandBuffer(pDevice->getGraphicsQueue(), pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);

		// Wait for command buffer to finish executing before deleting it
		pTempCommandBuffer->reset(true);
		m_pCommandPoolGraphics->freeCommandBuffer(&pTempCommandBuffer);
	} else {
		// Enable GPU-side computing
		CommandBufferVK* pTempCmdBufferCompute = m_ppCommandPools[0]->allocateCommandBuffer();
		pTempCmdBufferCompute->reset();
		pTempCmdBufferCompute->begin();

		for (ParticleEmitter* pEmitter : m_ParticleEmitters) {
			// Since ImGui is what triggered this, and ImGui is handled AFTER particles are updated, the particle buffers will be used
			// for rendering next, and the renderer will try to acquire ownership of the buffers for the rendering queue, the compute queue needs to release them
			BufferVK* pPositionsBuffer = reinterpret_cast<BufferVK*>(pEmitter->getPositionsBuffer());
			releaseFromCompute(pPositionsBuffer, pTempCmdBufferCompute);

			// Copy age and velocity data to the GPU buffers
			const ParticleStorage& particleStorage = pEmitter->getParticleStorage();

			BufferVK* pAgesBuffer = reinterpret_cast<BufferVK*>(pEmitter->getAgesBuffer());
			pTempCmdBufferCompute->updateBuffer(pAgesBuffer, 0, (const void*)particleStorage.ages.data(), particleStorage.ages.size() * sizeof(float));

			BufferVK* pVelocitiesBuffer = reinterpret_cast<BufferVK*>(pEmitter->getVelocitiesBuffer());
			pTempCmdBufferCompute->updateBuffer(pVelocitiesBuffer, 0, (const void*)particleStorage.velocities.data(), particleStorage.ages.size() * sizeof(glm::vec4));

			// Update emitter buffer
			EmitterBuffer emitterBuffer = {};
			pEmitter->createEmitterBuffer(emitterBuffer);

			BufferVK* pEmitterBuffer = reinterpret_cast<BufferVK*>(pEmitter->getEmitterBuffer());
			pTempCmdBufferCompute->updateBuffer(pEmitterBuffer, 0, &emitterBuffer, sizeof(EmitterBuffer));
		}

		pTempCmdBufferCompute->end();

		pDevice->executePrimaryCommandBuffer(pDevice->getComputeQueue(), pTempCmdBufferCompute, nullptr, nullptr, 0, nullptr, 0);

		// Wait for command buffer to finish executing before deleting it
		pTempCmdBufferCompute->reset(true);
		m_ppCommandPools[0]->freeCommandBuffer(&pTempCmdBufferCompute);
	}

	m_GPUComputed = !m_GPUComputed;
}

void ParticleEmitterHandlerVK::releaseFromGraphics(BufferVK* pBuffer, CommandBufferVK* pCommandBuffer)
{
	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
	DeviceVK* pDevice = pGraphicsContext->getDevice();
	const QueueFamilyIndices& queueFamilyIndices = pDevice->getQueueFamilyIndices();

	pCommandBuffer->releaseBufferOwnership(
		pBuffer,
		VK_ACCESS_SHADER_READ_BIT,
		queueFamilyIndices.graphicsFamily.value(),
		queueFamilyIndices.computeFamily.value(),
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
	);
}

void ParticleEmitterHandlerVK::releaseFromCompute(BufferVK* pBuffer, CommandBufferVK* pCommandBuffer)
{
	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
	DeviceVK* pDevice = pGraphicsContext->getDevice();
	const QueueFamilyIndices& queueFamilyIndices = pDevice->getQueueFamilyIndices();

	pCommandBuffer->releaseBufferOwnership(
		pBuffer,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		queueFamilyIndices.computeFamily.value(),
		queueFamilyIndices.graphicsFamily.value(),
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
	);
}

void ParticleEmitterHandlerVK::acquireForGraphics(BufferVK* pBuffer, CommandBufferVK* pCommandBuffer)
{
	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
	DeviceVK* pDevice = pGraphicsContext->getDevice();
	const QueueFamilyIndices& queueFamilyIndices = pDevice->getQueueFamilyIndices();

	pCommandBuffer->acquireBufferOwnership(
		pBuffer,
		VK_ACCESS_SHADER_READ_BIT,
		queueFamilyIndices.computeFamily.value(),
		queueFamilyIndices.graphicsFamily.value(),
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
	);
}

void ParticleEmitterHandlerVK::acquireForCompute(BufferVK* pBuffer, CommandBufferVK* pCommandBuffer)
{
	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
	DeviceVK* pDevice = pGraphicsContext->getDevice();
	const QueueFamilyIndices& queueFamilyIndices = pDevice->getQueueFamilyIndices();

	pCommandBuffer->acquireBufferOwnership(
		pBuffer,
		VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		queueFamilyIndices.graphicsFamily.value(),
		queueFamilyIndices.computeFamily.value(),
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
	);
}

void ParticleEmitterHandlerVK::initializeEmitter(ParticleEmitter* pEmitter)
{
	pEmitter->initialize(m_pGraphicsContext, m_pCamera);

	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
	DeviceVK* pDevice = pGraphicsContext->getDevice();
	const QueueFamilyIndices& queueFamilyIndices = pDevice->getQueueFamilyIndices();

	// Transfer ownership of buffers to compute queue
	if (queueFamilyIndices.graphicsFamily.value() != queueFamilyIndices.computeFamily.value()) {
		BufferVK* pVelocitiesBuffer = reinterpret_cast<BufferVK*>(pEmitter->getVelocitiesBuffer());
		BufferVK* pAgesBuffer = reinterpret_cast<BufferVK*>(pEmitter->getAgesBuffer());

		CommandBufferVK* pTempCommandBufferGraphics = m_pCommandPoolGraphics->allocateCommandBuffer();
		pTempCommandBufferGraphics->reset();
		pTempCommandBufferGraphics->begin();

		CommandBufferVK* pTempCommandBufferCompute = m_ppCommandPools[0]->allocateCommandBuffer();
		pTempCommandBufferCompute->reset();
		pTempCommandBufferCompute->begin();

		releaseFromGraphics(pVelocitiesBuffer, pTempCommandBufferGraphics);
		releaseFromGraphics(pAgesBuffer, pTempCommandBufferGraphics);

		// These only need to be acquired once, as the rendering stage does not use them
		acquireForCompute(pVelocitiesBuffer, pTempCommandBufferCompute);
		acquireForCompute(pAgesBuffer, pTempCommandBufferCompute);

		pTempCommandBufferGraphics->end();
		pDevice->executePrimaryCommandBuffer(pDevice->getGraphicsQueue(), pTempCommandBufferGraphics, nullptr, nullptr, 0, nullptr, 0);

		pTempCommandBufferCompute->end();
		pDevice->executePrimaryCommandBuffer(pDevice->getComputeQueue(), pTempCommandBufferCompute, nullptr, nullptr, 0, nullptr, 0);

		// Wait for command buffers to finish executing before deleting them
		pTempCommandBufferGraphics->reset(true);
		pTempCommandBufferCompute->reset(true);
		m_pCommandPoolGraphics->freeCommandBuffer(&pTempCommandBufferGraphics);
		m_ppCommandPools[0]->freeCommandBuffer(&pTempCommandBufferCompute);
	}
}

void ParticleEmitterHandlerVK::updateGPU(float dt)
{
	beginUpdateFrame();

	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
	DeviceVK* pDevice = pGraphicsContext->getDevice();
	const QueueFamilyIndices& queueFamilyIndices = pDevice->getQueueFamilyIndices();

	bool transferOwnerships = queueFamilyIndices.graphicsFamily.value() != queueFamilyIndices.computeFamily.value();
	m_ppCommandBuffers[m_CurrentFrame]->pushConstants(m_pPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), (const void*)&dt);

	for (ParticleEmitter* pEmitter : m_ParticleEmitters) {
		pEmitter->updateGPU(dt);

		// Update descriptor set
		BufferVK* pPositionsBuffer = reinterpret_cast<BufferVK*>(pEmitter->getPositionsBuffer());
		BufferVK* pVelocitiesBuffer = reinterpret_cast<BufferVK*>(pEmitter->getVelocitiesBuffer());
		BufferVK* pAgesBuffer = reinterpret_cast<BufferVK*>(pEmitter->getAgesBuffer());
		BufferVK* pEmitterBuffer = reinterpret_cast<BufferVK*>(pEmitter->getEmitterBuffer());

		if (transferOwnerships) {
			acquireForCompute(pPositionsBuffer, m_ppCommandBuffers[m_CurrentFrame]);
		}

		// TODO: Use constant variables or define macros for binding indices
		m_ppDescriptorSets[m_CurrentFrame]->writeStorageBufferDescriptor(pPositionsBuffer->getBuffer(), 0);
		m_ppDescriptorSets[m_CurrentFrame]->writeStorageBufferDescriptor(pVelocitiesBuffer->getBuffer(), 1);
		m_ppDescriptorSets[m_CurrentFrame]->writeStorageBufferDescriptor(pAgesBuffer->getBuffer(), 2);
		m_ppDescriptorSets[m_CurrentFrame]->writeUniformBufferDescriptor(pEmitterBuffer->getBuffer(), 3);

		uint32_t particleCount = pEmitter->getParticleCount();
		glm::u32vec3 workGroupSize(1 + particleCount / m_WorkGroupSize, 1, 1);

		m_pProfiler->writeTimestamp(&m_TimestampDispatch);
		m_ppCommandBuffers[m_CurrentFrame]->dispatch(workGroupSize);
		m_pProfiler->writeTimestamp(&m_TimestampDispatch);

		if (transferOwnerships) {
			releaseFromCompute(pPositionsBuffer, m_ppCommandBuffers[m_CurrentFrame]);
		}
    }

	endUpdateFrame();
}

void ParticleEmitterHandlerVK::beginUpdateFrame()
{
	m_ppCommandBuffers[m_CurrentFrame]->reset(true);
	m_ppCommandPools[m_CurrentFrame]->reset();

	m_ppCommandBuffers[m_CurrentFrame]->begin(nullptr);
	m_pProfiler->beginFrame(m_CurrentFrame);

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
	m_pProfiler->endFrame();

	m_ppCommandBuffers[m_CurrentFrame]->end();

	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
    DeviceVK* pDevice = pGraphicsContext->getDevice();

	pDevice->executePrimaryCommandBuffer(pDevice->getComputeQueue(), m_ppCommandBuffers[m_CurrentFrame], nullptr, nullptr, 0, nullptr, 0);
	m_pProfiler->writeResults();

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool ParticleEmitterHandlerVK::createCommandPoolAndBuffers()
{
    GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
    DeviceVK* pDevice = pGraphicsContext->getDevice();

	const QueueFamilyIndices& queueFamilyIndices = pDevice->getQueueFamilyIndices();
	const uint32_t computeQueueIndex = queueFamilyIndices.computeFamily.value();
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

	m_pCommandPoolGraphics = DBG_NEW CommandPoolVK(pDevice, queueFamilyIndices.graphicsFamily.value());
	return m_pCommandPoolGraphics->init();
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

	// Maximize the work group size
	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
    DeviceVK* pDevice = pGraphicsContext->getDevice();

	uint32_t pMaxWorkGroupSize[3];
	pDevice->getMaxComputeWorkGroupSize(pMaxWorkGroupSize);
	m_WorkGroupSize = pMaxWorkGroupSize[0];

	ShaderVK* pComputeShaderVK = reinterpret_cast<ShaderVK*>(pComputeShader);
	pComputeShaderVK->setSpecializationConstant(1, m_WorkGroupSize);

	m_pPipeline = DBG_NEW PipelineVK(pDevice);
	m_pPipeline->finalizeCompute(pComputeShader, m_pPipelineLayout);

	SAFEDELETE(pComputeShader);
	return true;
}

void ParticleEmitterHandlerVK::createProfiler()
{
	GraphicsContextVK* pGraphicsContext = reinterpret_cast<GraphicsContextVK*>(m_pGraphicsContext);
    DeviceVK* pDevice = pGraphicsContext->getDevice();

	m_pProfiler = DBG_NEW ProfilerVK("Particles Update", pDevice);
	m_pProfiler->init(m_ppCommandBuffers);

	m_pProfiler->initTimestamp(&m_TimestampDispatch, "Dispatch");
}
