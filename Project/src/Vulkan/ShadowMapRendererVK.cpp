#include "ShadowMapRendererVK.h"

#include "Common/IShader.h"
#include "Core/ParticleEmitter.h"
#include "Vulkan/BufferVK.h"
#include "Vulkan/CommandPoolVK.h"
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
#include "Vulkan/SwapChainVK.h"
#include "Vulkan/SceneVK.h"
#include "Vulkan/Texture2DVK.h"

#include <array>

ShadowMapRendererVK::ShadowMapRendererVK(GraphicsContextVK* pGraphicsContext, RenderingHandlerVK* pRenderingHandler)
	:m_pGraphicsContext(pGraphicsContext),
	m_pRenderingHandler(pRenderingHandler),
	m_pProfiler(nullptr),
	m_pPipeline(nullptr)
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_ppCommandPools[i] = nullptr;
		m_ppCommandBuffers[i] = nullptr;
	}
}

ShadowMapRendererVK::~ShadowMapRendererVK()
{
	SAFEDELETE(m_pProfiler);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		SAFEDELETE(m_ppCommandPools[i]);
	}

	SAFEDELETE(m_pPipeline);
}

bool ShadowMapRendererVK::init()
{
	if (!createCommandPoolAndBuffers()) {
		return false;
	}

	if (!createPipeline()) {
		return false;
	}

	createProfiler();

	return true;
}

void ShadowMapRendererVK::beginFrame(IScene* pScene)
{
	m_pScene = reinterpret_cast<SceneVK*>(pScene);

	DirectionalLight* pDirectionalLight = pScene->getLightSetup().getDirectionalLight();
	FrameBufferVK* pFrameBuffer = reinterpret_cast<FrameBufferVK*>(pDirectionalLight->getFrameBuffer());
	if (!pFrameBuffer && !createShadowMapResources(pDirectionalLight)) {
		LOG("Failed to create shadow map resources for directional light");
		return;
	}

	// Prepare for frame
	uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();

	m_ppCommandBuffers[frameIndex]->reset(false);
	m_ppCommandPools[frameIndex]->reset();

	// Needed to begin a secondary buffer
	VkCommandBufferInheritanceInfo inheritanceInfo = {};
	inheritanceInfo.sType		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.pNext		= nullptr;
	inheritanceInfo.renderPass	= m_pRenderingHandler->getParticleRenderPass()->getRenderPass();
	inheritanceInfo.subpass		= 0;
	inheritanceInfo.framebuffer = pFrameBuffer->getFrameBuffer();

	m_ppCommandBuffers[frameIndex]->begin(&inheritanceInfo, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
	m_pProfiler->beginFrame(m_ppCommandBuffers[frameIndex]);

	m_ppCommandBuffers[frameIndex]->setViewports(&m_Viewport, 1);
	m_ppCommandBuffers[frameIndex]->setScissorRects(&m_ScissorRect, 1);

	m_ppCommandBuffers[frameIndex]->bindPipeline(m_pPipeline);
}

void ShadowMapRendererVK::endFrame(IScene* pScene)
{
	UNREFERENCED_PARAMETER(pScene);

	uint32_t currentFrame = m_pRenderingHandler->getCurrentFrameIndex();

	m_pProfiler->endFrame();
	m_ppCommandBuffers[currentFrame]->end();
}

void ShadowMapRendererVK::renderUI()
{}

void ShadowMapRendererVK::submitMesh(const MeshVK* pMesh, const Material* pMaterial, uint32_t transformIndex)
{
	uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();

	SceneVK* pScene = reinterpret_cast<SceneVK*>(m_pRenderingHandler->getScene());
	m_ppCommandBuffers[frameIndex]->pushConstants(pScene->getGeometryPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &transformIndex);

	BufferVK* pIndexBuffer = reinterpret_cast<BufferVK*>(pMesh->getIndexBuffer());
	m_ppCommandBuffers[frameIndex]->bindIndexBuffer(pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	DescriptorSetVK* pDescriptorSet = m_pScene->getDescriptorSetFromMeshAndMaterial(pMesh, pMaterial);
	m_ppCommandBuffers[frameIndex]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pScene->getGeometryPipelineLayout(), 0, 1, &pDescriptorSet, 0, nullptr);

	m_ppCommandBuffers[frameIndex]->drawIndexInstanced(pMesh->getIndexCount(), 1, 0, 0, 0);
}

void ShadowMapRendererVK::setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY)
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

bool ShadowMapRendererVK::createCommandPoolAndBuffers()
{
	DeviceVK* pDevice		= m_pGraphicsContext->getDevice();

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

bool ShadowMapRendererVK::createPipeline()
{
	// Create pipeline state
	IShader* pVertexShader = m_pGraphicsContext->createShader();
	pVertexShader->initFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/shadowMapVertex.spv");
	if (!pVertexShader->finalize()) {
        LOG("Failed to create vertex shader for particle renderer");
		return false;
	}

	m_pPipeline = DBG_NEW PipelineVK(m_pGraphicsContext->getDevice());

	VkPipelineRasterizationStateCreateInfo rasterizerState = {};
	rasterizerState.cullMode	= VK_CULL_MODE_NONE;
	rasterizerState.frontFace	= VK_FRONT_FACE_CLOCKWISE;
	rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerState.lineWidth	= 1.0f;
	m_pPipeline->setRasterizerState(rasterizerState);

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.depthTestEnable	= VK_TRUE;
	depthStencilState.depthWriteEnable	= VK_TRUE;
	depthStencilState.depthCompareOp	= VK_COMPARE_OP_LESS;
	depthStencilState.stencilTestEnable	= VK_FALSE;
	m_pPipeline->setDepthStencilState(depthStencilState);

	SceneVK* pScene = reinterpret_cast<SceneVK*>(m_pRenderingHandler->getScene());
	m_pPipeline->finalizeGraphics({pVertexShader}, m_pRenderingHandler->getParticleRenderPass(), pScene->getGeometryPipelineLayout());
	SAFEDELETE(pVertexShader);

	return true;
}

void ShadowMapRendererVK::createProfiler()
{
	m_pProfiler = DBG_NEW ProfilerVK("Shadowmap Render", m_pGraphicsContext->getDevice());
}

bool ShadowMapRendererVK::createShadowMapResources(DirectionalLight* directionalLight)
{
	FrameBufferVK* pFrameBuffer = nullptr;
	ImageVK* pImage = nullptr;
	ImageViewVK* pImageView = nullptr;

    // Make the shadow map with the same resolution as the backbuffer
    VkExtent2D backbufferRes = m_pGraphicsContext->getSwapChain()->getExtent();

    ImageParams imageParams = {};
    imageParams.Type            = VK_IMAGE_TYPE_2D;
    imageParams.Samples         = VK_SAMPLE_COUNT_1_BIT;
    imageParams.Usage           = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageParams.MemoryProperty  = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imageParams.Format          = VK_FORMAT_D32_SFLOAT;
    imageParams.Extent          = {backbufferRes.width, backbufferRes.height, 1};
    imageParams.MipLevels       = 1;
    imageParams.ArrayLayers		= 1;

    // Create image view of the volumetric light image
    ImageViewParams imageViewParams = {};
	imageViewParams.Type			= VK_IMAGE_VIEW_TYPE_2D;
	imageViewParams.AspectFlags		= VK_IMAGE_ASPECT_DEPTH_BIT;
	imageViewParams.LayerCount		= 1;
	imageViewParams.MipLevels		= 1;

	pImage = DBG_NEW ImageVK(m_pGraphicsContext->getDevice());
	if (!pImage->init(imageParams)) {
		LOG("Failed to create shadow map image");
		return false;
	}

	pImageView = DBG_NEW ImageViewVK(m_pGraphicsContext->getDevice(), pImage);
	if (!pImageView->init(imageViewParams)) {
		LOG("Failed to create shadow map image view");
		return false;
	}

	// Create framebuffer
	pFrameBuffer = DBG_NEW FrameBufferVK(m_pGraphicsContext->getDevice());
	pFrameBuffer->addColorAttachment(pImageView);

	directionalLight->setFrameBuffer(pFrameBuffer);
	directionalLight->setDepthImage(pImage);
	directionalLight->setDepthImageView(pImageView);

	return true;
}
