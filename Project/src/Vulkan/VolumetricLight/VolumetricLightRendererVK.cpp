#include "VolumetricLightRendererVK.h"

#include "Vulkan/BufferVK.h"
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
#include "Vulkan/ImguiVK.h"
#include "Vulkan/MeshVK.h"
#include "Vulkan/PipelineLayoutVK.h"
#include "Vulkan/PipelineVK.h"
#include "Vulkan/RenderPassVK.h"
#include "Vulkan/RenderingHandlerVK.h"
#include "Vulkan/SamplerVK.h"
#include "Vulkan/SwapChainVK.h"
#include "Vulkan/SceneVK.h"

#define VERTEX_BINDING              0
#define VOLUMETRIC_LIGHT_BINDING    1
#define CAMERA_BINDING              2
#define DEPTH_BUFFER_BINDING        3

VolumetricLightRendererVK::VolumetricLightRendererVK(GraphicsContextVK* pGraphicsContext, RenderingHandlerVK* pRenderingHandler, LightSetup* pLightSetup, ImguiVK* pImguiRenderer)
    :m_pLightSetup(pLightSetup),
	m_pImguiRenderer(pImguiRenderer),
    m_pSphereMesh(nullptr),
	m_pLightBufferPass(nullptr),
	m_pLightFrameBuffer(nullptr),
    m_pGraphicsContext(pGraphicsContext),
    m_pRenderingHandler(pRenderingHandler),
    m_pProfiler(nullptr),
    m_pDescriptorPool(nullptr),
    m_pDescriptorSetLayoutCommon(nullptr),
    m_pDescriptorSetLayoutPerLight(nullptr),
	m_pDescriptorSetCommon(nullptr),
    m_pPipelineLayout(nullptr),
    m_pPipelinePointLight(nullptr),
	m_pPipelineDirectionalLight(nullptr),
    m_pSampler(nullptr),
	m_LightBufferImID(nullptr),
	m_CurrentIndex(0)
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_ppCommandBuffers[i] = nullptr;
        m_ppCommandPools[i] = nullptr;

		m_pLightBufferImage = nullptr;
		m_pLightBufferImageView = nullptr;
    }

	m_LightBufferClearColor = {};
	m_LightBufferClearColor.color.float32[0] = 0.0f;
	m_LightBufferClearColor.color.float32[1] = 0.0f;
	m_LightBufferClearColor.color.float32[2] = 0.0f;
	m_LightBufferClearColor.color.float32[3] = 1.0f;
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
	SAFEDELETE(m_pDescriptorSetLayoutCommon);
	SAFEDELETE(m_pDescriptorSetLayoutPerLight);
	SAFEDELETE(m_pDescriptorPool);
	SAFEDELETE(m_pPipelineLayout);
	SAFEDELETE(m_pPipelinePointLight);
	SAFEDELETE(m_pPipelineDirectionalLight);
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

	if (!createSphereMesh()) {
		return false;
	}

	if (!createPipelineLayout()) {
		return false;
	}

	if (!createPipelines()) {
		return false;
	}

	createProfiler();

	return true;
}

void VolumetricLightRendererVK::updateBuffers()
{
	std::vector<VolumetricPointLight>& volumetricPointLights = m_pLightSetup->getVolumetricPointLights();

	for (VolumetricPointLight& pointLight : volumetricPointLights) {
		if (!pointLight.getVolumetricLightBuffer()) {
			if (!createRenderResources(pointLight)) {
				return;
			}
		}

		if (pointLight.m_LightUpdated) {
			// Refresh the light's uniform buffer
			VolumetricPointLightBuffer buffer = {};
			pointLight.createVolumetricPointLightBuffer(buffer);

			CommandBufferVK* pCommandBuffer = m_pRenderingHandler->getCurrentGraphicsCommandBuffer();
			BufferVK* pDstBuffer = reinterpret_cast<BufferVK*>(pointLight.getVolumetricLightBuffer());

			pCommandBuffer->updateBuffer(pDstBuffer, 0, &buffer, sizeof(VolumetricPointLightBuffer));

			pointLight.m_LightUpdated = false;
		}
	}
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
	uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();

	// Update push constants
	PushConstants pushConstants = {};
	pushConstants.raymarchSteps = 64;
	pushConstants.viewportExtent = {m_Viewport.width, m_Viewport.height};
	m_ppCommandBuffers[frameIndex]->pushConstants(m_pPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);

	m_ppCommandBuffers[frameIndex]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 0, 1, &m_pDescriptorSetCommon, 0, nullptr);

	renderPointLights();
	renderDirectionalLight();
}

void VolumetricLightRendererVK::renderPointLights()
{
	std::vector<VolumetricPointLight>& volumetricPointLights = m_pLightSetup->getVolumetricPointLights();
	if (volumetricPointLights.empty()) {
		return;
	}

	uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();

	// Bind sphere mesh
	BufferVK* pIndexBuffer = reinterpret_cast<BufferVK*>(m_pSphereMesh->getIndexBuffer());
	m_ppCommandBuffers[frameIndex]->bindIndexBuffer(pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	m_ppCommandBuffers[frameIndex]->bindPipeline(m_pPipelinePointLight);

	for (VolumetricPointLight& pointLight : volumetricPointLights) {
		DescriptorSetVK* pDescriptorSet = reinterpret_cast<DescriptorSetVK*>(pointLight.getVolumetricLightDescriptorSet());
		m_ppCommandBuffers[frameIndex]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 1, 1, &pDescriptorSet, 0, nullptr);

		m_pProfiler->beginTimestamp(&m_TimestampDraw);
		m_ppCommandBuffers[frameIndex]->drawIndexInstanced(m_pSphereMesh->getIndexCount(), 1, 0, 0, 0);
		m_pProfiler->endTimestamp(&m_TimestampDraw);
	}
}

void VolumetricLightRendererVK::renderDirectionalLight()
{
	if (!m_pLightSetup->hasDirectionalLight()) {
		return;
	}

	uint32_t frameIndex = m_pRenderingHandler->getCurrentFrameIndex();

	m_ppCommandBuffers[frameIndex]->bindPipeline(m_pPipelineDirectionalLight);

	DirectionalLight* pDirectionalLight = m_pLightSetup->getDirectionalLight();
	DescriptorSetVK* pDescriptorSet = reinterpret_cast<DescriptorSetVK*>(pDirectionalLight->getDescriptorSet());
	m_ppCommandBuffers[frameIndex]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pPipelineLayout, 1, 1, &pDescriptorSet, 0, nullptr);

	m_pProfiler->beginTimestamp(&m_TimestampDraw);
	m_ppCommandBuffers[frameIndex]->drawInstanced(3, 1, 0, 0);
	m_pProfiler->endTimestamp(&m_TimestampDraw);
}

void VolumetricLightRendererVK::setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY)
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

void VolumetricLightRendererVK::onWindowResize(uint32_t width, uint32_t height)
{
	UNREFERENCED_PARAMETER(width);
	UNREFERENCED_PARAMETER(height);

	// Refresh volumetric light buffer
	SAFEDELETE(m_pLightBufferImage);
	SAFEDELETE(m_pLightBufferImageView);
	SAFEDELETE(m_pLightFrameBuffer);
	createFrameBuffer();

	// Delete volumetric light resources, they will be recreated during the next render call
	std::vector<VolumetricPointLight>& volumetricPointLights = m_pLightSetup->getVolumetricPointLights();
	for (VolumetricPointLight& pointLight : volumetricPointLights) {
		if (pointLight.getVolumetricLightBuffer()) {
			delete pointLight.getVolumetricLightBuffer();
			pointLight.setVolumetricLightBuffer(nullptr);
		}

		if (pointLight.getVolumetricLightDescriptorSet()) {
			delete pointLight.getVolumetricLightDescriptorSet();
			pointLight.setVolumetricLightDescriptorSet(nullptr);
		}
	}

	m_Viewport.width = (float)width;
	m_Viewport.height = (float)height;

	// Refresh depth buffer descriptor
	GBufferVK* pGBuffer = m_pRenderingHandler->getGBuffer();
	ImageViewVK* pDepthImageView = pGBuffer->getDepthAttachment();

	m_pDescriptorSetCommon->writeCombinedImageDescriptors(&pDepthImageView, &m_pSampler, 1, DEPTH_BUFFER_BINDING);
}

void VolumetricLightRendererVK::renderUI()
{
	ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);

	// Display light buffer
	if (ImGui::Begin("Volumetric Light", NULL, ImGuiWindowFlags_NoResize)) {
		// Point lights UI
		std::vector<VolumetricPointLight>& volumetricPointLights = m_pLightSetup->getVolumetricPointLights();
		if (!volumetricPointLights.empty()) {
			ImGui::Text("Point lights");
			ImGui::SliderInt("Light selection", &m_CurrentIndex, 0, int(volumetricPointLights.size() - 1));

			VolumetricPointLight& currentLight = volumetricPointLights[(size_t)m_CurrentIndex];
			float particleG = currentLight.getParticleG();
			float scatterAmount = currentLight.getScatterAmount();
			float radius = currentLight.getRadius();

			if (ImGui::SliderFloat("Particle G", &particleG, 0.0f, 1.0f)) {
				currentLight.setParticleG(particleG);
			}

			if (ImGui::SliderFloat("Scatter Amount", &scatterAmount, 0.0f, 1.0f)) {
				currentLight.setScatterAmount(scatterAmount);
			}

			if (ImGui::SliderFloat("Radius", &radius, 0.0f, 8.0f)) {
				currentLight.setRadius(radius);
			}

		}

		if (m_pLightSetup->hasDirectionalLight()) {
			ImGui::Text("Directional light");

			DirectionalLight* pDirectionalLight = m_pLightSetup->getDirectionalLight();
			float particleG = pDirectionalLight->getParticleG();
			float scatterAmount = pDirectionalLight->getScatterAmount();

			if (ImGui::SliderFloat("Particle G", &particleG, 0.0f, 1.0f)) {
				pDirectionalLight->setParticleG(particleG);
			}

			if (ImGui::SliderFloat("Scatter Amount", &scatterAmount, 0.0f, 1.0f)) {
				pDirectionalLight->setScatterAmount(scatterAmount);
			}
		}

		if (m_LightBufferImID) {
			// Display lightbuffer
			ImGui::Image(m_LightBufferImID, {m_Viewport.width * 0.25f, m_Viewport.height * 0.25f});
		}
	}

	ImGui::End();
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

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment	= 0;
	colorAttachmentRef.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	m_pLightBufferPass->addSubpass(&colorAttachmentRef, 1, nullptr);

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
    imageParams.Extent          = {backbufferRes.width, backbufferRes.height, 1};
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

	// Create ImGui texture ID for light buffer
	m_LightBufferImID = m_pImguiRenderer->addTexture(m_pLightBufferImageView);

	// Create framebuffer
	m_pLightFrameBuffer = DBG_NEW FrameBufferVK(m_pGraphicsContext->getDevice());
	m_pLightFrameBuffer->addColorAttachment(m_pLightBufferImageView);

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
	m_pDescriptorSetLayoutCommon = DBG_NEW DescriptorSetLayoutVK(pDevice);
	m_pDescriptorSetLayoutPerLight = DBG_NEW DescriptorSetLayoutVK(pDevice);

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.size = sizeof(PushConstants);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;

	m_pDescriptorSetLayoutCommon->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT, VERTEX_BINDING, 1);
	m_pDescriptorSetLayoutCommon->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, CAMERA_BINDING, 1);
	m_pDescriptorSetLayoutCommon->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, &sampler, DEPTH_BUFFER_BINDING, 1);
	m_pDescriptorSetLayoutPerLight->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, LIGHT_BUFFER_BINDING, 1);
	m_pDescriptorSetLayoutPerLight->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, &sampler, SHADOW_MAP_BINDING, 1);

	if (!m_pDescriptorSetLayoutCommon->finalize() || !m_pDescriptorSetLayoutPerLight->finalize()) {
		LOG("Failed to finalize descriptor set layout");
		return false;
	}

	std::vector<const DescriptorSetLayoutVK*> descriptorSetLayouts = { m_pDescriptorSetLayoutCommon, m_pDescriptorSetLayoutPerLight };

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

	// Create common descriptor set
	GBufferVK* pGBuffer = m_pRenderingHandler->getGBuffer();
	ImageViewVK* pDepthImageView = pGBuffer->getDepthAttachment();

	BufferVK* pVertexBuffer = reinterpret_cast<BufferVK*>(m_pSphereMesh->getVertexBuffer());

	m_pDescriptorSetCommon = m_pDescriptorPool->allocDescriptorSet(m_pDescriptorSetLayoutCommon);
	m_pDescriptorSetCommon->writeStorageBufferDescriptor(pVertexBuffer, VERTEX_BINDING);
	m_pDescriptorSetCommon->writeUniformBufferDescriptor(m_pRenderingHandler->getCameraBufferGraphics(), CAMERA_BINDING);
	m_pDescriptorSetCommon->writeCombinedImageDescriptors(&pDepthImageView, &m_pSampler, 1, DEPTH_BUFFER_BINDING);

	m_pPipelineLayout = DBG_NEW PipelineLayoutVK(pDevice);
	return m_pPipelineLayout->init(descriptorSetLayouts, {pushConstantRange});
}

bool VolumetricLightRendererVK::createPipelines()
{
	// Create pipeline state for point lights
	IShader* pVertexShader = m_pGraphicsContext->createShader();
	pVertexShader->initFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/volumetricLight/volumetricPointVertex.spv");
	if (!pVertexShader->finalize()) {
        LOG("Failed to create vertex shader for volumetric point lights");
		return false;
	}

	IShader* pPixelShader = m_pGraphicsContext->createShader();
	pPixelShader->initFromFile(EShader::PIXEL_SHADER, "main", "assets/shaders/volumetricLight/volumetricPointFragment.spv");
	if (!pPixelShader->finalize()) {
		LOG("Failed to create pixel shader for volumetric point lights");
		return false;
	}

	std::vector<const IShader*> shaders = { pVertexShader, pPixelShader };
	m_pPipelinePointLight = DBG_NEW PipelineVK(m_pGraphicsContext->getDevice());

	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.blendEnable			= VK_FALSE;
	blendAttachment.colorWriteMask		= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_pPipelinePointLight->addColorBlendAttachment(blendAttachment);

	VkPipelineRasterizationStateCreateInfo rasterizerState = {};
	rasterizerState.cullMode	= VK_CULL_MODE_FRONT_BIT;
	rasterizerState.frontFace	= VK_FRONT_FACE_CLOCKWISE;
	rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerState.lineWidth	= 1.0f;
	m_pPipelinePointLight->setRasterizerState(rasterizerState);

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.depthTestEnable	= VK_FALSE;
	depthStencilState.depthWriteEnable	= VK_FALSE;
	m_pPipelinePointLight->setDepthStencilState(depthStencilState);

	if (!m_pPipelinePointLight->finalizeGraphics(shaders, m_pLightBufferPass, m_pPipelineLayout)) {
		LOG("Failed to create volumetric point light pipeline");
		return false;
	}

	SAFEDELETE(pVertexShader);
	SAFEDELETE(pPixelShader);

	// Create pipeline state for directional light
	pVertexShader = m_pGraphicsContext->createShader();
	pVertexShader->initFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/fullscreenVertex.spv");
	if (!pVertexShader->finalize()) {
        LOG("Failed to create vertex shader for volumetric directional light");
		return false;
	}

	pPixelShader = m_pGraphicsContext->createShader();
	pPixelShader->initFromFile(EShader::PIXEL_SHADER, "main", "assets/shaders/volumetricLight/volumetricDirectionalFragment.spv");
	if (!pPixelShader->finalize()) {
		LOG("Failed to create pixel shader for volumetric directional light");
		return false;
	}

	shaders = { pVertexShader, pPixelShader };
	m_pPipelineDirectionalLight = DBG_NEW PipelineVK(m_pGraphicsContext->getDevice());

	m_pPipelineDirectionalLight->addColorBlendAttachment(blendAttachment);

	rasterizerState.cullMode = VK_CULL_MODE_NONE;
	m_pPipelineDirectionalLight->setRasterizerState(rasterizerState);

	m_pPipelineDirectionalLight->setDepthStencilState(depthStencilState);

	if (!m_pPipelineDirectionalLight->finalizeGraphics(shaders, m_pLightBufferPass, m_pPipelineLayout)) {
		LOG("Failed to create volumetric directional light pipeline");
		return false;
	}

	SAFEDELETE(pVertexShader);
	SAFEDELETE(pPixelShader);

	return true;
}

bool VolumetricLightRendererVK::createSphereMesh()
{
	m_pSphereMesh = DBG_NEW MeshVK(m_pGraphicsContext->getDevice());
	return m_pSphereMesh->initFromFile("assets/meshes/sphere2.obj");
}

void VolumetricLightRendererVK::createProfiler()
{
	m_pProfiler = DBG_NEW ProfilerVK("Volumetric Light Renderer", m_pGraphicsContext->getDevice());
	m_pProfiler->initTimestamp(&m_TimestampDraw, "Draw");
}

bool VolumetricLightRendererVK::createRenderResources(VolumetricPointLight& pointLight)
{
	// Create volumetric point light buffer
	VolumetricPointLightBuffer buffer = {};
	pointLight.createVolumetricPointLightBuffer(buffer);

	BufferVK* pLightBuffer = DBG_NEW BufferVK(m_pGraphicsContext->getDevice());

	BufferParams bufferParams = {};
	bufferParams.SizeInBytes = sizeof(VolumetricPointLightBuffer);
	bufferParams.Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	bufferParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	bufferParams.IsExclusive = true;
	if (!pLightBuffer->init(bufferParams)) {
		LOG("Failed to create volumetric point light buffer");
		return false;
	}

	// Set buffer data
	CommandBufferVK* pCommandBuffer = m_pRenderingHandler->getCurrentGraphicsCommandBuffer();
	pCommandBuffer->updateBuffer(pLightBuffer, 0, &buffer, sizeof(VolumetricPointLightBuffer));

    // Create descriptor set
    DescriptorSetVK* pDescriptorSet = m_pDescriptorPool->allocDescriptorSet(m_pDescriptorSetLayoutPerLight);
	pDescriptorSet->writeUniformBufferDescriptor(pLightBuffer, VOLUMETRIC_LIGHT_BINDING);
    pointLight.setVolumetricLightDescriptorSet(pDescriptorSet);

	pointLight.setVolumetricLightBuffer(pLightBuffer);
	return true;
}
