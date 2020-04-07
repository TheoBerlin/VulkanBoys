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
#include "Vulkan/ImageViewVK.h"
#include "Vulkan/MeshVK.h"
#include "Vulkan/Particles/ParticleEmitterHandlerVK.h"
#include "Vulkan/PipelineLayoutVK.h"
#include "Vulkan/PipelineVK.h"
#include "Vulkan/RenderingHandlerVK.h"
#include "Vulkan/RenderPassVK.h"
#include "Vulkan/SamplerVK.h"
#include "Vulkan/Texture2DVK.h"

#include <array>

#define BINDING_VERTEX 				0
#define BINDING_CAMERA 				1
#define BINDING_EMITTER 			2
#define BINDING_PARTICLE_POSITIONS	3
#define BINDING_PARTICLE_TEXTURE	4

ParticleRendererVK::ParticleRendererVK(GraphicsContextVK* pGraphicsContext, RenderingHandlerVK* pRenderingHandler)
	:m_pGraphicsContext(pGraphicsContext),
	m_pRenderingHandler(pRenderingHandler),
	m_pProfiler(nullptr),
	m_pDescriptorPool(nullptr),
	m_pDescriptorSetLayout(nullptr),
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
	SAFEDELETE(m_pProfiler);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		SAFEDELETE(m_ppCommandPools[i]);
	}

	SAFEDELETE(m_pDescriptorSetLayout);
	SAFEDELETE(m_pDescriptorPool);
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

	createProfiler();

	return true;
}

void ParticleRendererVK::beginFrame(IScene* pScene)
{
	UNREFERENCED_PARAMETER(pScene);

	// Prepare for frame
	uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();

	m_ppCommandBuffers[frameIndex]->reset(false);
	m_ppCommandPools[frameIndex]->reset();

	m_pProfiler->reset(frameIndex, m_pRenderingHandler->getCurrentGraphicsCommandBuffer());

	// Needed to begin a secondary buffer
	VkCommandBufferInheritanceInfo inheritanceInfo = {};
	inheritanceInfo.sType		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.pNext		= nullptr;
	inheritanceInfo.renderPass	= m_pRenderingHandler->getParticleRenderPass()->getRenderPass();
	inheritanceInfo.subpass		= 0; // TODO: Don't hardcode this :(
	inheritanceInfo.framebuffer = m_pRenderingHandler->getCurrentBackBufferWithDepth()->getFrameBuffer();

	m_ppCommandBuffers[frameIndex]->begin(&inheritanceInfo, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
	m_pProfiler->beginFrame(m_ppCommandBuffers[frameIndex]);

	m_ppCommandBuffers[frameIndex]->setViewports(&m_Viewport, 1);
	m_ppCommandBuffers[frameIndex]->setScissorRects(&m_ScissorRect, 1);

	// Bind quad
	BufferVK* pIndexBuffer = reinterpret_cast<BufferVK*>(m_pQuadMesh->getIndexBuffer());
	m_ppCommandBuffers[frameIndex]->bindIndexBuffer(pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
	m_ppCommandBuffers[frameIndex]->bindPipeline(m_pPipeline);
}

void ParticleRendererVK::endFrame(IScene* pScene)
{
	UNREFERENCED_PARAMETER(pScene);

	uint32_t currentFrame = m_pRenderingHandler->getCurrentFrameIndex();

	m_pProfiler->endFrame();
	m_ppCommandBuffers[currentFrame]->end();
}

void ParticleRendererVK::renderUI()
{
}

void ParticleRendererVK::submitParticles(ParticleEmitter* pEmitter)
{
	uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();

	if (!bindDescriptorSet(pEmitter)) {
		return;
	}

	uint32_t particleCount = pEmitter->getParticleCount();

	m_pProfiler->beginTimestamp(&m_TimestampDraw);
	m_ppCommandBuffers[frameIndex]->drawIndexInstanced(m_pQuadMesh->getIndexCount(), particleCount, 0, 0, 0);
	m_pProfiler->endTimestamp(&m_TimestampDraw);
}

void ParticleRendererVK::setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY)
{
	m_Viewport.x		= topX;
	m_Viewport.y		= topY;
	m_Viewport.width	= width;
	m_Viewport.height	= height;
	m_Viewport.minDepth = minDepth;
	m_Viewport.maxDepth = maxDepth;

	m_ScissorRect.extent.width	= (uint32_t)width;
	m_ScissorRect.extent.height = (uint32_t)height;
	m_ScissorRect.offset.x		= 0;
	m_ScissorRect.offset.y		= 0;
}

bool ParticleRendererVK::createCommandPoolAndBuffers()
{
	DeviceVK* pDevice = m_pGraphicsContext->getDevice();

	const uint32_t graphicsQueueIndex = pDevice->getQueueFamilyIndices().graphicsFamily.value();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
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

	// Particle Texture
	m_pSampler = DBG_NEW SamplerVK(pDevice);

	SamplerParams samplerParams = {};
	samplerParams.MinFilter = VkFilter::VK_FILTER_LINEAR;
	samplerParams.MagFilter = VkFilter::VK_FILTER_LINEAR;
	samplerParams.WrapModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerParams.WrapModeV = samplerParams.WrapModeU;
	samplerParams.WrapModeW = samplerParams.WrapModeU;
	m_pSampler->init(samplerParams);

	VkSampler sampler = m_pSampler->getSampler();

	// Descriptor Set Layout
	m_pDescriptorSetLayout = DBG_NEW DescriptorSetLayoutVK(pDevice);

	m_pDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT, 			BINDING_VERTEX, 1);
	m_pDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 			BINDING_CAMERA, 1);
	m_pDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 			BINDING_EMITTER, 1);
	m_pDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT, 			BINDING_PARTICLE_POSITIONS, 1);
	m_pDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, &sampler, BINDING_PARTICLE_TEXTURE, 1);

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
	if (!m_pDescriptorPool->init(descriptorCounts, 256)) {
		LOG("Failed to initialize descriptor pool");
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
        LOG("Failed to create vertex shader for particle renderer");
		return false;
	}

	IShader* pPixelShader = m_pGraphicsContext->createShader();
	pPixelShader->initFromFile(EShader::PIXEL_SHADER, "main", "assets/shaders/particles/fragment.spv");
	if (!pPixelShader->finalize()) {
		LOG("Failed to create pixel shader for particle renderer");
		return false;
	}

	std::vector<const IShader*> shaders = { pVertexShader, pPixelShader };
	m_pPipeline = DBG_NEW PipelineVK(m_pGraphicsContext->getDevice());

	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.blendEnable			= VK_TRUE;
	blendAttachment.colorWriteMask		= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachment.colorBlendOp		= VK_BLEND_OP_ADD;
	blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachment.alphaBlendOp		= VK_BLEND_OP_ADD;
	m_pPipeline->addColorBlendAttachment(blendAttachment);

	VkPipelineRasterizationStateCreateInfo rasterizerState = {};
	rasterizerState.cullMode	= VK_CULL_MODE_NONE;
	rasterizerState.frontFace	= VK_FRONT_FACE_CLOCKWISE;
	rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerState.lineWidth	= 1.0f;
	m_pPipeline->setRasterizerState(rasterizerState);

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.depthTestEnable	= VK_TRUE;
	depthStencilState.depthWriteEnable	= VK_FALSE;
	depthStencilState.depthCompareOp	= VK_COMPARE_OP_LESS;
	depthStencilState.stencilTestEnable	= VK_FALSE;
	m_pPipeline->setDepthStencilState(depthStencilState);

	m_pPipeline->finalizeGraphics(shaders, m_pRenderingHandler->getParticleRenderPass(), m_pPipelineLayout);

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
	return m_pQuadMesh->initFromMemory(pQuadVertices.data(), sizeof(QuadVertex), uint32_t(pQuadVertices.size()), pQuadIndices.data(), uint32_t(pQuadIndices.size()));
}

void ParticleRendererVK::createProfiler()
{
	m_pProfiler = DBG_NEW ProfilerVK("Particles Renderer", m_pGraphicsContext->getDevice());
	m_pProfiler->initTimestamp(&m_TimestampDraw, "Draw Instanced");
}

bool ParticleRendererVK::bindDescriptorSet(ParticleEmitter* pEmitter)
{
	if (pEmitter->getDescriptorSetRender() == nullptr) {
		// Create a descriptor set for the emitter
		DescriptorSetVK* pDescriptorSet = m_pDescriptorPool->allocDescriptorSet(m_pDescriptorSetLayout);
		if (pDescriptorSet == nullptr) {
			LOG("Failed to create descriptor set for particle renderer");
			return false;
		}

		BufferVK* pEmitterBuffer = reinterpret_cast<BufferVK*>(pEmitter->getEmitterBuffer());
		BufferVK* pPositionsBuffer = reinterpret_cast<BufferVK*>(pEmitter->getPositionsBuffer());
		Texture2DVK* pParticleTexture = reinterpret_cast<Texture2DVK*>(pEmitter->getParticleTexture());

		// Storage buffer for quad vertices
		BufferVK* pVertBuffer = reinterpret_cast<BufferVK*>(m_pQuadMesh->getVertexBuffer());

		// Camera buffers
		BufferVK* pCameraBuffer = m_pRenderingHandler->getCameraBufferGraphics();

		pDescriptorSet->writeStorageBufferDescriptor(pVertBuffer, 		BINDING_VERTEX);
		pDescriptorSet->writeUniformBufferDescriptor(pCameraBuffer, 	BINDING_CAMERA);
		pDescriptorSet->writeUniformBufferDescriptor(pEmitterBuffer, 	BINDING_EMITTER);
		pDescriptorSet->writeStorageBufferDescriptor(pPositionsBuffer,	BINDING_PARTICLE_POSITIONS);

		ImageViewVK* pParticleTextureVIew = pParticleTexture->getImageView();
		pDescriptorSet->writeCombinedImageDescriptors(&pParticleTextureVIew, &m_pSampler, 1, BINDING_PARTICLE_TEXTURE);

		pEmitter->setDescriptorSetRender(pDescriptorSet);
	}

	// Bind descriptor set
	uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();

	DescriptorSetVK* pDescriptorSet = reinterpret_cast<DescriptorSetVK*>(pEmitter->getDescriptorSetRender());
	m_ppCommandBuffers[frameIndex]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &pDescriptorSet, 0, nullptr);

	return true;
}
