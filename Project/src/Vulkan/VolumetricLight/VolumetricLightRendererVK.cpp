#include "VolumetricLightRendererVK.h"

#include "Core/PointLight.h"
#include "Vulkan/CommandBufferVK.h"
#include "Vulkan/CommandPoolVK.h"
#include "Vulkan/DescriptorPoolVK.h"
#include "Vulkan/DescriptorSetLayoutVK.h"
#include "Vulkan/DescriptorSetVK.h"
#include "Vulkan/FrameBufferVK.h"
#include "Vulkan/GBufferVK.h"
#include "Vulkan/GraphicsContextVK.h"
#include "Vulkan/ImageViewVK.h"
#include "Vulkan/ImageVK.h"
#include "Vulkan/MeshVK.h"
#include "Vulkan/PipelineLayoutVK.h"
#include "Vulkan/PipelineVK.h"
#include "Vulkan/RenderPassVK.h"
#include "Vulkan/RenderingHandlerVK.h"
#include "Vulkan/SamplerVK.h"
#include "Vulkan/SwapChainVK.h"
#include "Vulkan/SceneVK.h"

#define VERTEX_BINDING              0
#define CAMERA_BINDING              1
#define DEPTH_BUFFER_BINDING        2
//#define VOLUMETRIC_LIGHT_BINDING    3

VolumetricLightRendererVK::VolumetricLightRendererVK(GraphicsContextVK* pGraphicsContext, RenderingHandlerVK* pRenderingHandler, LightSetup* pLightSetup)
    :m_pLightSetup(pLightSetup),
    m_pSphereMesh(nullptr),
	m_pLightBufferPass(nullptr),
	m_pLightFrameBuffer(nullptr),
    m_pGraphicsContext(pGraphicsContext),
    m_pRenderingHandler(pRenderingHandler),
    m_pProfiler(nullptr),
    m_pDescriptorPool(nullptr),
    m_pDescriptorSetLayout(nullptr),
    m_pPipelineLayout(nullptr),
    m_pPipeline(nullptr),
    m_pSampler(nullptr)
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_ppCommandBuffers[i] = nullptr;
        m_ppCommandPools[i] = nullptr;

		m_pLightBufferImage = nullptr;
		m_pLightBufferImageView = nullptr;
    }
}

VolumetricLightRendererVK::~VolumetricLightRendererVK()
{
    SAFEDELETE(m_pProfiler);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		SAFEDELETE(m_ppCommandPools[i]);

	}

	SAFEDELETE(m_pLightBufferPass);
	SAFEDELETE(m_pLightBufferImageView);
	SAFEDELETE(m_pLightBufferImage);
	SAFEDELETE(m_pLightFrameBuffer);
	SAFEDELETE(m_pDescriptorSetLayout);
	SAFEDELETE(m_pDescriptorPool);
	SAFEDELETE(m_pPipelineLayout);
	SAFEDELETE(m_pPipeline);
	SAFEDELETE(m_pSphereMesh);
	SAFEDELETE(m_pSampler);
}

bool VolumetricLightRendererVK::init()
{
    if (!createCommandPoolAndBuffers()) {
		return false;
	}

	if (!createRenderPass()) {
		return false;
	}

	if (!createFrameBuffer()) {
		return false;
	}

	if (!createPipelineLayout()) {
		return false;
	}

	if (!createPipeline()) {
		return false;
	}

	if (!createSphereMesh()) {
		return false;
	}

	createProfiler();

	return true;
}

void VolumetricLightRendererVK::beginFrame(IScene* pScene)
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
	inheritanceInfo.renderPass	= m_pLightBufferPass->getRenderPass();
	inheritanceInfo.subpass		= 0; // TODO: Don't hardcode this :(
	inheritanceInfo.framebuffer = m_pLightFrameBuffer->getFrameBuffer();

	m_ppCommandBuffers[frameIndex]->begin(&inheritanceInfo, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
	m_pProfiler->beginFrame(m_ppCommandBuffers[frameIndex]);

	m_ppCommandBuffers[frameIndex]->setViewports(&m_Viewport, 1);
	m_ppCommandBuffers[frameIndex]->setScissorRects(&m_ScissorRect, 1);

	// Bind sphere mesh
	BufferVK* pIndexBuffer = reinterpret_cast<BufferVK*>(m_pSphereMesh->getIndexBuffer());
	m_ppCommandBuffers[frameIndex]->bindIndexBuffer(pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	m_ppCommandBuffers[frameIndex]->bindPipeline(m_pPipeline);
}

void VolumetricLightRendererVK::endFrame(IScene* pScene)
{
	UNREFERENCED_PARAMETER(pScene);

	m_pProfiler->endFrame();

	uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();
	m_ppCommandBuffers[frameIndex]->end();
}

void VolumetricLightRendererVK::renderLightBuffer()
{
	// Begin light buffer pass
	VkClearValue clearColor = {};
	clearColor.color.float32[0] = 0.0f;
	clearColor.color.float32[1] = 0.0f;
	clearColor.color.float32[2] = 0.0f;
	clearColor.color.float32[3] = 1.0f;

	uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();
	m_ppCommandBuffers[frameIndex]->beginRenderPass(m_pLightBufferPass, m_pLightFrameBuffer, (uint32_t)m_Viewport.width, (uint32_t)m_Viewport.height, &clearColor, 1, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	const std::vector<VolumetricPointLight>& volumetricPointLights = m_pLightSetup->getVolumetricPointLights();

	PushConstants pushConstants = {};
	pushConstants.raymarchSteps = 32;

	for (VolumetricPointLight pointLight : volumetricPointLights) {

		// if (!bindDescriptorSet(pPointLight)) {
		// 	return;
		// }

		// Update push constants
		pushConstants.worldMatrix = glm::scale(glm::vec3(pointLight.getRadius())) * glm::translate(pointLight.getPosition());
		pushConstants.lightPosition = pointLight.getPosition();
		pushConstants.lightColor = pointLight.getColor();
		pushConstants.lightRadius = pointLight.getRadius();
		pushConstants.lightScatterAmount = pointLight.getScatterAmount();
		pushConstants.particleG = pointLight.getParticleG();
		m_ppCommandBuffers[frameIndex]->pushConstants(m_pPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);

		m_pProfiler->beginTimestamp(&m_TimestampDraw);
		m_ppCommandBuffers[frameIndex]->drawIndexInstanced(m_pSphereMesh->getIndexCount(), 1, 0, 0, 0);
		m_pProfiler->endTimestamp(&m_TimestampDraw);
	}

	m_ppCommandBuffers[frameIndex]->endRenderPass();
}

// void VolumetricLightRendererVK::submitPointLight(PointLight* pPointLight)
// {
//     uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();

// }

void VolumetricLightRendererVK::setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY)
{
    m_Viewport.x		= topX;
	m_Viewport.y		= topY;
	m_Viewport.width	= width / 2;
	m_Viewport.height	= height / 2;
	m_Viewport.minDepth = minDepth;
	m_Viewport.maxDepth = maxDepth;

	m_ScissorRect.extent.width	= (uint32_t)width / 2;
	m_ScissorRect.extent.height = (uint32_t)height / 2;
	m_ScissorRect.offset.x		= 0;
	m_ScissorRect.offset.y		= 0;
}

bool VolumetricLightRendererVK::createCommandPoolAndBuffers()
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

bool VolumetricLightRendererVK::createRenderPass()
{
	m_pLightBufferPass = DBG_NEW RenderPassVK(m_pGraphicsContext->getDevice());

	// Light buffer
	VkAttachmentDescription description = {};
	description.format			= VK_FORMAT_R8G8B8A8_UNORM;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_pLightBufferPass->addAttachment(description);

	// Depth buffer
	description.format			= VK_FORMAT_D32_SFLOAT;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD;
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	description.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pLightBufferPass->addAttachment(description);

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment	= 0;
	colorAttachmentRef.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentRef = {};
	depthStencilAttachmentRef.attachment	= 1;
	depthStencilAttachmentRef.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	m_pLightBufferPass->addSubpass(&colorAttachmentRef, 1, &depthStencilAttachmentRef);

	VkSubpassDependency dependency = {};
	dependency.srcSubpass		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass		= 0;
	dependency.dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;
	dependency.srcAccessMask	= 0;
	dependency.dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	m_pLightBufferPass->addSubpassDependency(dependency);

	if (!m_pLightBufferPass->finalize()) {
		LOG("Failed to create volumetric light buffer pass");
		return false;
	}

	return true;
}

bool VolumetricLightRendererVK::createFrameBuffer()
{
	// Make the image half of the backbuffer's resolution
	VkExtent2D backbufferRes = m_pGraphicsContext->getSwapChain()->getExtent();

    ImageParams imageParams = {};
    imageParams.Type            = VK_IMAGE_TYPE_2D;
    imageParams.Samples         = VK_SAMPLE_COUNT_1_BIT;
    imageParams.Usage           = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageParams.MemoryProperty  = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imageParams.Format          = VK_FORMAT_R8G8B8A8_UNORM;
    imageParams.Extent          = {backbufferRes.width / 2, backbufferRes.height / 2, 1};
    imageParams.MipLevels       = 1;
    imageParams.ArrayLayers		= 1;

    // Create image view of the volumetric light image
    ImageViewParams imageViewParams = {};
	imageViewParams.Type			= VK_IMAGE_VIEW_TYPE_2D;
	imageViewParams.AspectFlags		= VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewParams.LayerCount		= 1;
	imageViewParams.MipLevels		= 1;

	m_pLightBufferImage = DBG_NEW ImageVK(m_pGraphicsContext->getDevice());
	if (!m_pLightBufferImage->init(imageParams)) {
		LOG("Failed to create volumetric light image");
		return false;
	}

	m_pLightBufferImageView = DBG_NEW ImageViewVK(m_pGraphicsContext->getDevice(), m_pLightBufferImage);
	if (!m_pLightBufferImageView->init(imageViewParams)) {
		LOG("Failed to create volumetric lighting image view");
		return false;
	}

	GBufferVK* pGBuffer = m_pRenderingHandler->getGBuffer();

	m_pLightFrameBuffer = DBG_NEW FrameBufferVK(m_pGraphicsContext->getDevice());
	m_pLightFrameBuffer->addColorAttachment(m_pLightBufferImageView);
	m_pLightFrameBuffer->setDepthStencilAttachment(pGBuffer->getDepthImageView());

	return m_pLightFrameBuffer->finalize(m_pLightBufferPass, imageParams.Extent.width, imageParams.Extent.height);
}

bool VolumetricLightRendererVK::createPipelineLayout()
{
	DeviceVK* pDevice = m_pGraphicsContext->getDevice();

	// Depth buffer sampler
	m_pSampler = new SamplerVK(pDevice);

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

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.size = sizeof(PushConstants);
	//pushConstantRange.size = sizeof(glm::mat4) + sizeof(glm::vec3) * 2 + sizeof(float) * 3 + sizeof(uint32_t);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;

	m_pDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT, VERTEX_BINDING, 1);
	m_pDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, CAMERA_BINDING, 1);
	m_pDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, &sampler, DEPTH_BUFFER_BINDING, 1);

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
	return m_pPipelineLayout->init(descriptorSetLayouts, {pushConstantRange});
}

bool VolumetricLightRendererVK::createPipeline()
{
	// Create pipeline state
	IShader* pVertexShader = m_pGraphicsContext->createShader();
	pVertexShader->initFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/volumetricLight/volumetricVertex.spv");
	if (!pVertexShader->finalize()) {
        LOG("Failed to create vertex shader for volumetric light renderer");
		return false;
	}

	IShader* pPixelShader = m_pGraphicsContext->createShader();
	pPixelShader->initFromFile(EShader::PIXEL_SHADER, "main", "assets/shaders/volumetricLight/volumetricFragment.spv");
	if (!pPixelShader->finalize()) {
		LOG("Failed to create pixel shader for volumetric light renderer");
		return false;
	}

	std::vector<const IShader*> shaders = { pVertexShader, pPixelShader };
	m_pPipeline = DBG_NEW PipelineVK(m_pGraphicsContext->getDevice());

	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.blendEnable			= VK_TRUE;
	blendAttachment.colorWriteMask		= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
	blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachment.colorBlendOp		= VK_BLEND_OP_ADD;
	blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachment.alphaBlendOp		= VK_BLEND_OP_ADD;
	m_pPipeline->addColorBlendAttachment(blendAttachment);

	VkPipelineRasterizationStateCreateInfo rasterizerState = {};
	rasterizerState.cullMode	= VK_CULL_MODE_BACK_BIT;
	rasterizerState.frontFace	= VK_FRONT_FACE_CLOCKWISE;
	rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerState.lineWidth	= 1.0f;
	m_pPipeline->setRasterizerState(rasterizerState);

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.depthTestEnable	= VK_FALSE;
	depthStencilState.depthWriteEnable	= VK_FALSE;
	m_pPipeline->setDepthStencilState(depthStencilState);

	m_pPipeline->finalizeGraphics(shaders, m_pRenderingHandler->getBackBufferRenderPass(), m_pPipelineLayout);

	SAFEDELETE(pVertexShader);
	SAFEDELETE(pPixelShader);

	return true;
}

bool VolumetricLightRendererVK::createSphereMesh()
{
	m_pSphereMesh = DBG_NEW MeshVK(m_pGraphicsContext->getDevice());
    return m_pSphereMesh->initAsSphere(2);
}

void VolumetricLightRendererVK::createProfiler()
{
	m_pProfiler = DBG_NEW ProfilerVK("Volumetric Light Renderer", m_pGraphicsContext->getDevice());
	m_pProfiler->initTimestamp(&m_TimestampDraw, "Draw");
}

// bool VolumetricLightRendererVK::bindDescriptorSet(PointLight* pPointLight)
// {
//     if (!pPointLight->getVolumetricLightImage()) {
//         if (createRenderResources(pPointLight)) {
//             return false;
//         }
//     }

//     // Bind descriptor set
// 	uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();

// 	DescriptorSetVK* pDescriptorSet = reinterpret_cast<DescriptorSetVK*>(pPointLight->getVolumetricLightDescriptorSet());
// 	m_ppCommandBuffers[frameIndex]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &pDescriptorSet, 0, nullptr);

// 	return true;
// }

// bool VolumetricLightRendererVK::createRenderResources(PointLight* pPointLight)
// {
//     // Create volumetric light image
//     ImageVK* pVolumetricLightImage = DBG_NEW ImageVK(m_pGraphicsContext->getDevice());

//     ImageParams imageParams = {};
//     imageParams.Type            = VK_IMAGE_TYPE_3D;
//     imageParams.Samples         = VK_SAMPLE_COUNT_1_BIT;
//     imageParams.Usage           = VK_IMAGE_USAGE_STORAGE_BIT;
//     imageParams.MemoryProperty  = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
//     imageParams.Format          = VK_FORMAT_R16G16B16A16_SFLOAT;
//     imageParams.Extent          = {128, 64, 128};
//     imageParams.MipLevels       = 1;

//     if (!pVolumetricLightImage->init(imageParams)) {
//         LOG("Failed to create volumetric lighting image");
//         return false;
//     }

//     // Create image view of the volumetric light image
//     ImageViewParams imageViewParams = {};
// 	imageViewParams.Type			= VK_IMAGE_VIEW_TYPE_3D;
// 	imageViewParams.AspectFlags		= VK_IMAGE_ASPECT_COLOR_BIT;
// 	imageViewParams.LayerCount		= 1;
// 	imageViewParams.MipLevels		= 1;

//     ImageViewVK* pVolumetricLightImageView = DBG_NEW ImageViewVK(m_pGraphicsContext->getDevice(), pVolumetricLightImage);
//     if (!pVolumetricLightImageView->init(imageViewParams)) {
//         LOG("Failed to create volumetric lighting image view");
//         return false;
//     }

//     pPointLight->setVolumetricLightImage(pVolumetricLightImage);
//     pPointLight->setVolumetricLightImageView(pVolumetricLightImageView);

//     // Create descriptor set
//     DescriptorSetVK* pDescriptorSet = m_pDescriptorPool->allocDescriptorSet(m_pDescriptorSetLayout);
//     pDescriptorSet->writeStorageImageDescriptor(pVolumetricLightImageView, VOLUMETRIC_LIGHT_BINDING);
//     pPointLight->setVolumetricLightDescriptorSet(pDescriptorSet);

//     return true;
// }
