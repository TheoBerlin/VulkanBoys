#include "RendererVK.h"
#include "MeshVK.h"
#include "ImguiVK.h"
#include "BufferVK.h"
#include "GBufferVK.h"
#include "SamplerVK.h"
#include "PipelineVK.h"
#include "SwapChainVK.h"
#include "Texture2DVK.h"
#include "ImageViewVK.h"
#include "RenderPassVK.h"
#include "CommandPoolVK.h"
#include "FrameBufferVK.h"
#include "TextureCubeVK.h"
#include "DescriptorSetVK.h"
#include "CommandBufferVK.h"
#include "DescriptorPoolVK.h"
#include "PipelineLayoutVK.h"
#include "GraphicsContextVK.h"
#include "SkyboxRendererVK.h"
#include "TextureCubeVK.h"

#include "Core/Camera.h"
#include "Core/LightSetup.h"

#include <glm/gtc/type_ptr.hpp>

RendererVK::RendererVK(GraphicsContextVK* pContext)
	: m_pContext(pContext),
	m_ppCommandPools(),
	m_ppCommandBuffers(),
	m_pRenderPass(nullptr),
	m_ppBackbuffers(),
	m_pSkyboxPipeline(nullptr),
	m_pLightDescriptorSet(nullptr),
	m_pGBufferSampler(nullptr),
	m_pSkyboxPipelineLayout(nullptr),
	m_pGBuffer(nullptr),
	m_pDescriptorPool(nullptr),
	m_pLightDescriptorSetLayout(nullptr),
	m_pCameraBuffer(nullptr),
	m_pLightBuffer(nullptr),
	m_ClearColor(),
	m_ClearDepth(),
	m_Viewport(),
	m_ScissorRect(),
	m_CurrentFrame(0)
{
	m_ClearDepth.depthStencil.depth = 1.0f;
	m_ClearDepth.depthStencil.stencil = 0;
}

RendererVK::~RendererVK()
{
	m_pContext->getDevice()->wait();

	releaseFramebuffers();

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		SAFEDELETE(m_ppCommandPools[i]);
		if (m_ImageAvailableSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(m_pContext->getDevice()->getDevice(), m_ImageAvailableSemaphores[i], nullptr);
		}
		if (m_RenderFinishedSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(m_pContext->getDevice()->getDevice(), m_RenderFinishedSemaphores[i], nullptr);
		}
	}

	SAFEDELETE(m_pSkyboxSampler);
	SAFEDELETE(m_pSkyboxPipeline);
	SAFEDELETE(m_pSkyboxPipelineLayout);
	SAFEDELETE(m_pSkyboxDescriptorSetLayout);
	SAFEDELETE(m_pSkyboxRenderer);
	SAFEDELETE(m_pGBuffer);
	SAFEDELETE(m_pGBufferSampler);
	SAFEDELETE(m_pRenderPass);
	SAFEDELETE(m_pBackBufferRenderPass);
	SAFEDELETE(m_pSkyboxPipelineLayout);
	SAFEDELETE(m_pSkyboxPipeline);
	SAFEDELETE(m_pGeometryPipeline);
	SAFEDELETE(m_pGeometryPipelineLayout);
	SAFEDELETE(m_pGeometryDescriptorSetLayout);
	SAFEDELETE(m_pLightPipeline);
	SAFEDELETE(m_pLightPipelineLayout);
	SAFEDELETE(m_pDescriptorPool);
	SAFEDELETE(m_pLightDescriptorSetLayout);
	SAFEDELETE(m_pCameraBuffer);
	SAFEDELETE(m_pLightBuffer);
	SAFEDELETE(m_pDefaultTexture);
	SAFEDELETE(m_pDefaultNormal);

	m_pContext = nullptr;
}

bool RendererVK::init()
{
	DeviceVK* pDevice = m_pContext->getDevice();
	SwapChainVK* pSwapChain = m_pContext->getSwapChain();

	if (!createSamplers())
	{
		return false;
	}

	m_pSkyboxRenderer = DBG_NEW SkyboxRendererVK(m_pContext->getDevice());
	if (!m_pSkyboxRenderer->init())
	{
		return false;
	}

	if (!createRenderPass())
	{
		return false;
	}

	if (!createGBuffer())
	{
		return false;
	}

	createFramebuffers();

	if (!createCommandPoolAndBuffers())
	{
		return false;
	}

	if (!createPipelineLayouts())
	{
		return false;
	}

	if (!createPipelines())
	{
		return false;
	}

	if (!createSemaphores())
	{
		return false;
	}

	if (!createBuffersAndTextures())
	{
		return false;
	}

	m_pLightDescriptorSet->writeCombinedImageDescriptor(m_pGBuffer->getColorAttachment(0), m_pGBufferSampler, GBUFFER_ALBEDO_BINDING);
	m_pLightDescriptorSet->writeCombinedImageDescriptor(m_pGBuffer->getColorAttachment(1), m_pGBufferSampler, GBUFFER_NORMAL_BINDING);
	m_pLightDescriptorSet->writeCombinedImageDescriptor(m_pGBuffer->getColorAttachment(2), m_pGBufferSampler, GBUFFER_POSITION_BINDING);
	m_pLightDescriptorSet->writeUniformBufferDescriptor(m_pLightBuffer, LIGHT_BUFFER_BINDING);
	m_pLightDescriptorSet->writeUniformBufferDescriptor(m_pCameraBuffer, CAMERA_BUFFER_BINDING);

	m_pSkyboxDescriptorSet->writeUniformBufferDescriptor(m_pCameraBuffer, 0);

	return true;
}

ITextureCube* RendererVK::generateTextureCubeFromPanorama(ITexture2D* pPanorama, uint32_t width, uint32_t miplevels, ETextureFormat format)
{
	TextureCubeVK* pTextureCube = DBG_NEW TextureCubeVK(m_pContext->getDevice());
	pTextureCube->init(width, miplevels, format);
	
	Texture2DVK* pPanoramaVK = reinterpret_cast<Texture2DVK*>(pPanorama);
	m_pSkyboxRenderer->generateCubemapFromPanorama(pTextureCube, pPanoramaVK);

	return pTextureCube;
}

void RendererVK::onWindowResize(uint32_t width, uint32_t height)
{
	m_pContext->getDevice()->wait();
	releaseFramebuffers();
	
	m_pContext->getSwapChain()->resize(width, height);

	m_pGBuffer->resize(width, height);
	m_pLightDescriptorSet->writeCombinedImageDescriptor(m_pGBuffer->getColorAttachment(0), m_pGBufferSampler, GBUFFER_ALBEDO_BINDING);
	m_pLightDescriptorSet->writeCombinedImageDescriptor(m_pGBuffer->getColorAttachment(1), m_pGBufferSampler, GBUFFER_NORMAL_BINDING);
	m_pLightDescriptorSet->writeCombinedImageDescriptor(m_pGBuffer->getColorAttachment(2), m_pGBufferSampler, GBUFFER_POSITION_BINDING);

	createFramebuffers();
}

void RendererVK::beginFrame(const Camera& camera, const LightSetup& lightSetup)
{
	//Prepare for frame
	m_pContext->getSwapChain()->acquireNextImage(m_ImageAvailableSemaphores[m_CurrentFrame]);

	m_ppCommandBuffers[m_CurrentFrame]->reset();
	m_ppCommandPools[m_CurrentFrame]->reset();

	m_ppCommandBuffers[m_CurrentFrame]->begin();

	//Update camera
	CameraBuffer cameraBuffer = {};
	cameraBuffer.Projection = camera.getProjectionMat();
	cameraBuffer.View		= camera.getViewMat();
	cameraBuffer.Position	= glm::vec4(camera.getPosition(), 1.0f);
	m_ppCommandBuffers[m_CurrentFrame]->updateBuffer(m_pCameraBuffer, 0, (const void*)&cameraBuffer, sizeof(CameraBuffer));
	//Update lights
	const uint32_t numPointLights = lightSetup.getPointLightCount();
	m_ppCommandBuffers[m_CurrentFrame]->updateBuffer(m_pLightBuffer, 0, (const void*)lightSetup.getPointLights(), sizeof(PointLight) * numPointLights);

	//Begin geometrypass
	VkClearValue clearValues[] = { m_ClearColor, m_ClearColor, m_ClearColor, m_ClearDepth };
	m_ppCommandBuffers[m_CurrentFrame]->beginRenderPass(m_pRenderPass, m_pGBuffer->getFrameBuffer(), m_Viewport.width, m_Viewport.height, clearValues, 4);

	m_ppCommandBuffers[m_CurrentFrame]->setViewports(&m_Viewport, 1);
	m_ppCommandBuffers[m_CurrentFrame]->setScissorRects(&m_ScissorRect, 1);
}

void RendererVK::endFrame()
{
	m_ppCommandBuffers[m_CurrentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pSkyboxPipelineLayout, 0, 1, &m_pSkyboxDescriptorSet, 0, nullptr);
	m_ppCommandBuffers[m_CurrentFrame]->bindGraphicsPipeline(m_pSkyboxPipeline);
	m_ppCommandBuffers[m_CurrentFrame]->drawInstanced(36, 1, 0, 0);

	m_ppCommandBuffers[m_CurrentFrame]->endRenderPass();

	//Begin lightpass
	VkClearValue clearValues[] = { m_ClearColor, m_ClearDepth };
	uint32_t backBufferIndex = m_pContext->getSwapChain()->getImageIndex();
	m_ppCommandBuffers[m_CurrentFrame]->beginRenderPass(m_pBackBufferRenderPass, m_ppBackbuffers[backBufferIndex], m_Viewport.width, m_Viewport.height, clearValues, 2);
	
	m_ppCommandBuffers[m_CurrentFrame]->bindGraphicsPipeline(m_pLightPipeline);
	m_ppCommandBuffers[m_CurrentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pLightPipelineLayout, 0, 1, &m_pLightDescriptorSet, 0, nullptr);

	m_ppCommandBuffers[m_CurrentFrame]->setViewports(&m_Viewport, 1);
	m_ppCommandBuffers[m_CurrentFrame]->setScissorRects(&m_ScissorRect, 1);

	m_ppCommandBuffers[m_CurrentFrame]->drawInstanced(3, 1, 0, 0);

	m_ppCommandBuffers[m_CurrentFrame]->endRenderPass();

	m_ppCommandBuffers[m_CurrentFrame]->end();

	//Execute commandbuffer
	VkSemaphore waitSemaphores[]		= { m_ImageAvailableSemaphores[m_CurrentFrame] };
	VkSemaphore signalSemaphores[]		= { m_RenderFinishedSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags waitStages[]	= { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	DeviceVK* pDevice = m_pContext->getDevice();
	pDevice->executeCommandBuffer(pDevice->getGraphicsQueue(), m_ppCommandBuffers[m_CurrentFrame], waitSemaphores, waitStages, 1, signalSemaphores, 1);
}

void RendererVK::setClearColor(float r, float g, float b)
{
	setClearColor(glm::vec3(r, g, b));
}

void RendererVK::setClearColor(const glm::vec3& color)
{
	m_ClearColor.color.float32[0] = color.r;
	m_ClearColor.color.float32[1] = color.g;
	m_ClearColor.color.float32[2] = color.b;
	m_ClearColor.color.float32[3] = 1.0f;
}

void RendererVK::setSkybox(ITextureCube* pSkybox)
{
	m_pSkybox = reinterpret_cast<TextureCubeVK*>(pSkybox);
	m_pSkyboxDescriptorSet->writeCombinedImageDescriptor(m_pSkybox->getImageView(), m_pSkyboxSampler, 1);
}

void RendererVK::setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY)
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

void RendererVK::swapBuffers()
{
	m_pContext->swapBuffers(m_RenderFinishedSemaphores[m_CurrentFrame]);
	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RendererVK::submitMesh(IMesh* pMesh, const Material& material, const glm::mat4& transform)
{
	ASSERT(pMesh != nullptr);

	m_ppCommandBuffers[m_CurrentFrame]->bindGraphicsPipeline(m_pGeometryPipeline);

	m_ppCommandBuffers[m_CurrentFrame]->pushConstants(m_pGeometryPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::mat4), (const void*)glm::value_ptr(transform));
	m_ppCommandBuffers[m_CurrentFrame]->pushConstants(m_pGeometryPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(glm::vec4), (const void*)glm::value_ptr(material.getAlbedo()));

	glm::vec3 materialProperties(material.getAmbientOcclusion(), material.getMetallic(), material.getRoughness());
	m_ppCommandBuffers[m_CurrentFrame]->pushConstants(m_pGeometryPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4) + sizeof(glm::vec4), sizeof(glm::vec3), (const void*)glm::value_ptr(materialProperties));

	BufferVK* pIndexBuffer = reinterpret_cast<BufferVK*>(pMesh->getIndexBuffer());
	m_ppCommandBuffers[m_CurrentFrame]->bindIndexBuffer(pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

	DescriptorSetVK* pDescriptorSet = getDescriptorSetFromMeshAndMaterial(pMesh, &material);
	m_ppCommandBuffers[m_CurrentFrame]->bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pGeometryPipelineLayout, 0, 1, &pDescriptorSet, 0, nullptr);

	m_ppCommandBuffers[m_CurrentFrame]->drawIndexInstanced(pMesh->getIndexCount(), 1, 0, 0, 0);
}

void RendererVK::drawImgui(IImgui* pImgui)
{
	pImgui->render(m_ppCommandBuffers[m_CurrentFrame]);
}

bool RendererVK::createGBuffer()
{
	VkExtent2D extent = m_pContext->getSwapChain()->getExtent();

	m_pGBuffer = DBG_NEW GBufferVK(m_pContext->getDevice());
	m_pGBuffer->addColorAttachmentFormat(VK_FORMAT_R8G8B8A8_UNORM);
	m_pGBuffer->addColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
	m_pGBuffer->addColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
	m_pGBuffer->setDepthAttachmentFormat(VK_FORMAT_D24_UNORM_S8_UINT);
	return m_pGBuffer->finalize(m_pRenderPass, extent.width, extent.height);
}

bool RendererVK::createSemaphores()
{
	//Create semaphores
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(m_pContext->getDevice()->getDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]), "Failed to create semaphores for a Frame");
		VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(m_pContext->getDevice()->getDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]), "Failed to create semaphores for a Frame");
	}

	return true;
}

bool RendererVK::createCommandPoolAndBuffers()
{
	DeviceVK* pDevice = m_pContext->getDevice();

	const uint32_t queueFamilyIndex = pDevice->getQueueFamilyIndices().graphicsFamily.value();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, queueFamilyIndex);
		
		if (!m_ppCommandPools[i]->init())
		{
			return false;
		}

		m_ppCommandBuffers[i] = m_ppCommandPools[i]->allocateCommandBuffer();
		if (m_ppCommandBuffers[i] == nullptr)
		{
			return false;
		}
	}

	return true;
}

void RendererVK::createFramebuffers()
{
	SwapChainVK* pSwapChain = m_pContext->getSwapChain();
	DeviceVK* pDevice = m_pContext->getDevice();

	VkExtent2D extent = pSwapChain->getExtent();
	ImageViewVK* pDepthStencilView = pSwapChain->getDepthStencilView();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppBackbuffers[i] = DBG_NEW FrameBufferVK(pDevice);
		m_ppBackbuffers[i]->addColorAttachment(pSwapChain->getImageView(i));
		m_ppBackbuffers[i]->setDepthStencilAttachment(pDepthStencilView);
		m_ppBackbuffers[i]->finalize(m_pBackBufferRenderPass, extent.width, extent.height);
	}
}

void RendererVK::releaseFramebuffers()
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		SAFEDELETE(m_ppBackbuffers[i]);
	}
}

bool RendererVK::createRenderPass()
{
	//Create renderpass
	m_pBackBufferRenderPass = DBG_NEW RenderPassVK(m_pContext->getDevice());
	VkAttachmentDescription description = {};
	description.format			= VK_FORMAT_B8G8R8A8_UNORM;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	m_pBackBufferRenderPass->addAttachment(description);

	description.format			= VK_FORMAT_D24_UNORM_S8_UINT;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pBackBufferRenderPass->addAttachment(description);

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment	= 0;
	colorAttachmentRef.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentRef = {};
	depthStencilAttachmentRef.attachment	= 1;
	depthStencilAttachmentRef.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pBackBufferRenderPass->addSubpass(&colorAttachmentRef, 1, &depthStencilAttachmentRef);

	VkSubpassDependency dependency = {};
	dependency.dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;
	dependency.srcSubpass		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass		= 0;
	dependency.srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask	= 0;
	dependency.dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	m_pBackBufferRenderPass->addSubpassDependency(dependency);
	if (!m_pBackBufferRenderPass->finalize())
	{
		return false;
	}

	//Create renderpass
	m_pRenderPass = DBG_NEW RenderPassVK(m_pContext->getDevice());

	//Albedo
	description = {};
	description.format			= VK_FORMAT_R8G8B8A8_UNORM;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;				
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;				
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;	
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;	
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;			
	description.finalLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_pRenderPass->addAttachment(description);

	//Normals
	description = {};
	description.format			= VK_FORMAT_R16G16B16A16_SFLOAT;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;				
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;				
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;	
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;	
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;			
	description.finalLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_pRenderPass->addAttachment(description);

	//World position
	description = {};
	description.format			= VK_FORMAT_R16G16B16A16_SFLOAT;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_pRenderPass->addAttachment(description);

	//Depth
	description.format			= VK_FORMAT_D24_UNORM_S8_UINT;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;				
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;				
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;	
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;	
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;							
	description.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pRenderPass->addAttachment(description);

	VkAttachmentReference colorAttachmentRefs[3];
	//Albedo
	colorAttachmentRefs[0].attachment	= 0;
	colorAttachmentRefs[0].layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//Normals
	colorAttachmentRefs[1].attachment	= 1;
	colorAttachmentRefs[1].layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//Positions
	colorAttachmentRefs[2].attachment	= 2;
	colorAttachmentRefs[2].layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	depthStencilAttachmentRef = {};
	depthStencilAttachmentRef.attachment	= 3;
	depthStencilAttachmentRef.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	m_pRenderPass->addSubpass(colorAttachmentRefs, 3, &depthStencilAttachmentRef);

	dependency = {};
	dependency.dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;
	dependency.srcSubpass		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass		= 0;
	dependency.srcStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
	dependency.dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	m_pRenderPass->addSubpassDependency(dependency);
	
	dependency.srcSubpass		= 0;
	dependency.dstSubpass		= VK_SUBPASS_EXTERNAL;
	dependency.srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
	m_pRenderPass->addSubpassDependency(dependency);
	
	return m_pRenderPass->finalize();
}

bool RendererVK::createPipelines()
{
	//Geometry Pass
	IShader* pVertexShader = m_pContext->createShader();
	pVertexShader->initFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/geometryVertex.spv");
	if (!pVertexShader->finalize())
	{
		return false;
	}

	IShader* pPixelShader = m_pContext->createShader();
	pPixelShader->initFromFile(EShader::PIXEL_SHADER, "main", "assets/shaders/geometryFragment.spv");
	if (!pPixelShader->finalize())
	{
		return false;
	}

	m_pGeometryPipeline = DBG_NEW PipelineVK(m_pContext->getDevice());
	
	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.blendEnable		= VK_FALSE;
	blendAttachment.colorWriteMask	= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_pGeometryPipeline->addColorBlendAttachment(blendAttachment);
	m_pGeometryPipeline->addColorBlendAttachment(blendAttachment);
	m_pGeometryPipeline->addColorBlendAttachment(blendAttachment);

	VkPipelineRasterizationStateCreateInfo rasterizerState = {};
	rasterizerState.cullMode	= VK_CULL_MODE_BACK_BIT;
	rasterizerState.frontFace	= VK_FRONT_FACE_CLOCKWISE;
	rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerState.lineWidth	= 1.0f;
	m_pGeometryPipeline->setRasterizerState(rasterizerState);

	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.depthTestEnable	= VK_TRUE;
	depthStencilState.depthWriteEnable	= VK_TRUE;
	depthStencilState.depthCompareOp	= VK_COMPARE_OP_LESS;
	depthStencilState.stencilTestEnable = VK_FALSE;
	m_pGeometryPipeline->setDepthStencilState(depthStencilState);

	std::vector<IShader*> shaders = { pVertexShader, pPixelShader };
	if (!m_pGeometryPipeline->finalize(shaders, m_pRenderPass, m_pGeometryPipelineLayout))
	{
		return false;
	}

	SAFEDELETE(pVertexShader);
	SAFEDELETE(pPixelShader);

	//Light Pass
	pVertexShader = m_pContext->createShader();
	pVertexShader->initFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/lightVertex.spv");
	if (!pVertexShader->finalize())
	{
		return false;
	}

	pPixelShader = m_pContext->createShader();
	pPixelShader->initFromFile(EShader::PIXEL_SHADER, "main", "assets/shaders/lightFragment.spv");
	if (!pPixelShader->finalize())
	{
		return false;
	}

	shaders = { pVertexShader, pPixelShader };
	m_pLightPipeline = DBG_NEW PipelineVK(m_pContext->getDevice());

	blendAttachment.blendEnable = VK_FALSE;
	blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_pLightPipeline->addColorBlendAttachment(blendAttachment);

	rasterizerState.cullMode	= VK_CULL_MODE_NONE;
	rasterizerState.lineWidth	= 1.0f;
	m_pLightPipeline->setRasterizerState(rasterizerState);

	depthStencilState.depthTestEnable    = VK_TRUE;
	depthStencilState.depthWriteEnable	 = VK_TRUE;
	depthStencilState.depthCompareOp	 = VK_COMPARE_OP_LESS;
	depthStencilState.stencilTestEnable	 = VK_FALSE;
	m_pLightPipeline->setDepthStencilState(depthStencilState);
	if (!m_pLightPipeline->finalize(shaders, m_pBackBufferRenderPass, m_pLightPipelineLayout))
	{
		return false;
	}

	SAFEDELETE(pVertexShader);
	SAFEDELETE(pPixelShader);

	//Skybox
	pVertexShader = m_pContext->createShader();
	pVertexShader->initFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/skyboxVertex.spv");
	if (!pVertexShader->finalize())
	{
		return false;
	}

	pPixelShader = m_pContext->createShader();
	pPixelShader->initFromFile(EShader::PIXEL_SHADER, "main", "assets/shaders/skyboxFragment.spv");
	if (!pPixelShader->finalize())
	{
		return false;
	}

	shaders = { pVertexShader, pPixelShader };
	m_pSkyboxPipeline = DBG_NEW PipelineVK(m_pContext->getDevice());

	blendAttachment = {};
	blendAttachment.blendEnable		= VK_FALSE;
	blendAttachment.colorWriteMask	= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_pSkyboxPipeline->addColorBlendAttachment(blendAttachment);
	m_pSkyboxPipeline->addColorBlendAttachment(blendAttachment);
	m_pSkyboxPipeline->addColorBlendAttachment(blendAttachment);

	rasterizerState = {};
	rasterizerState.cullMode	= VK_CULL_MODE_BACK_BIT;
	rasterizerState.frontFace	= VK_FRONT_FACE_CLOCKWISE;
	rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerState.lineWidth	= 1.0f;
	m_pSkyboxPipeline->setRasterizerState(rasterizerState);

	depthStencilState = {};
	depthStencilState.depthTestEnable	= VK_TRUE;
	depthStencilState.depthWriteEnable	= VK_TRUE;
	depthStencilState.depthCompareOp	= VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.stencilTestEnable = VK_FALSE;
	m_pSkyboxPipeline->setDepthStencilState(depthStencilState);
	if (!m_pSkyboxPipeline->finalize(shaders, m_pRenderPass, m_pSkyboxPipelineLayout))
	{
		return false;
	}

	SAFEDELETE(pVertexShader);
	SAFEDELETE(pPixelShader);

	return true;
}

bool RendererVK::createPipelineLayouts()
{
	//Descriptorpool
	DescriptorCounts descriptorCounts = {};
	descriptorCounts.m_SampledImages = 128;
	descriptorCounts.m_StorageBuffers = 128;
	descriptorCounts.m_UniformBuffers = 128;

	m_pDescriptorPool = DBG_NEW DescriptorPoolVK(m_pContext->getDevice());
	if (!m_pDescriptorPool->init(descriptorCounts, 16))
	{
		return false;
	}

	//GeometryPass
	m_pGeometryDescriptorSetLayout = DBG_NEW DescriptorSetLayoutVK(m_pContext->getDevice());
	m_pGeometryDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, CAMERA_BUFFER_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT, VERTEX_BUFFER_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, ALBEDO_MAP_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, NORMAL_MAP_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, AO_MAP_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, METALLIC_MAP_BINDING, 1);
	m_pGeometryDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, ROUGHNESS_MAP_BINDING, 1);
	
	if (!m_pGeometryDescriptorSetLayout->finalize())
	{
		return false;
	}	
	
	//Transform and Color
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.size = sizeof(glm::mat4) + sizeof(glm::vec4) + sizeof(glm::vec3);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	std::vector<VkPushConstantRange> pushConstantRanges = { pushConstantRange };
	std::vector<const DescriptorSetLayoutVK*> descriptorSetLayouts = { m_pGeometryDescriptorSetLayout };

	m_pGeometryPipelineLayout = DBG_NEW PipelineLayoutVK(m_pContext->getDevice());
	if (!m_pGeometryPipelineLayout->init(descriptorSetLayouts, pushConstantRanges))
	{
		return false;
	}

	//Lightpass
	m_pLightDescriptorSetLayout = DBG_NEW DescriptorSetLayoutVK(m_pContext->getDevice());
	m_pLightDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, LIGHT_BUFFER_BINDING, 1);
	m_pLightDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, CAMERA_BUFFER_BINDING, 1);
	m_pLightDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, GBUFFER_ALBEDO_BINDING, 1);
	m_pLightDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, GBUFFER_NORMAL_BINDING, 1);
	m_pLightDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, GBUFFER_POSITION_BINDING, 1);	
	if (!m_pLightDescriptorSetLayout->finalize())
	{
		return false;
	}

	pushConstantRanges = { };
	descriptorSetLayouts = { m_pLightDescriptorSetLayout };

	m_pLightPipelineLayout = DBG_NEW PipelineLayoutVK(m_pContext->getDevice());
	if (!m_pLightPipelineLayout->init(descriptorSetLayouts, pushConstantRanges))
	{
		return false;
	}

	m_pLightDescriptorSet = m_pDescriptorPool->allocDescriptorSet(m_pLightDescriptorSetLayout);
	if (!m_pLightDescriptorSet)
	{
		return false;
	}

	//Skybox
	m_pSkyboxDescriptorSetLayout = DBG_NEW DescriptorSetLayoutVK(m_pContext->getDevice());
	m_pSkyboxDescriptorSetLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 0, 1);
	m_pSkyboxDescriptorSetLayout->addBindingCombinedImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, 1, 1);
	if (!m_pSkyboxDescriptorSetLayout->finalize())
	{
		return false;
	}

	pushConstantRanges = { };
	descriptorSetLayouts = { m_pSkyboxDescriptorSetLayout };

	m_pSkyboxPipelineLayout = DBG_NEW PipelineLayoutVK(m_pContext->getDevice());
	if (!m_pSkyboxPipelineLayout->init(descriptorSetLayouts, pushConstantRanges))
	{
		return false;
	}

	m_pSkyboxDescriptorSet = m_pDescriptorPool->allocDescriptorSet(m_pSkyboxDescriptorSetLayout);
	if (!m_pSkyboxDescriptorSet)
	{
		return false;
	}

	return true;
}

bool RendererVK::createBuffersAndTextures()
{
	BufferParams cameraBufferParams = {};
	cameraBufferParams.Usage			= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	cameraBufferParams.SizeInBytes		= sizeof(CameraBuffer);
	cameraBufferParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	m_pCameraBuffer = DBG_NEW BufferVK(m_pContext->getDevice());
	if (!m_pCameraBuffer->init(cameraBufferParams))
	{
		return false;
	}

	BufferParams lightBufferParams = {};
	lightBufferParams.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	lightBufferParams.SizeInBytes = sizeof(PointLight) * MAX_POINTLIGHTS;
	lightBufferParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	m_pLightBuffer = DBG_NEW BufferVK(m_pContext->getDevice());
	if (!m_pLightBuffer->init(lightBufferParams))
	{
		return false;
	}

	uint8_t whitePixels[] = { 255, 255, 255, 255 };
	m_pDefaultTexture = DBG_NEW Texture2DVK(m_pContext->getDevice());
	if (!m_pDefaultTexture->initFromMemory(whitePixels, 1, 1, ETextureFormat::FORMAT_R8G8B8A8_UNORM, false))
	{
		return false;
	}

	uint8_t pixels[] = { 127, 127, 255, 255 };
	m_pDefaultNormal = DBG_NEW Texture2DVK(m_pContext->getDevice());
	return m_pDefaultNormal->initFromMemory(pixels, 1, 1, ETextureFormat::FORMAT_R8G8B8A8_UNORM, false);
}

bool RendererVK::createSamplers()
{
	SamplerParams params = {};
	params.MagFilter = VK_FILTER_NEAREST;
	params.MinFilter = VK_FILTER_NEAREST;
	params.WrapModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	params.WrapModeV = params.WrapModeU;
	params.WrapModeW = params.WrapModeU;

	m_pGBufferSampler = DBG_NEW SamplerVK(m_pContext->getDevice());
	if (!m_pGBufferSampler->init(params))
	{
		return false;
	}

	m_pSkyboxSampler = DBG_NEW SamplerVK(m_pContext->getDevice());
	if (!m_pSkyboxSampler->init(params))
	{
		return false;
	}

	return true;
}

DescriptorSetVK* RendererVK::getDescriptorSetFromMeshAndMaterial(const IMesh* pMesh, const Material* pMaterial)
{
	MeshFilter filter = {};
	filter.pMesh = pMesh;
	filter.pMaterial = pMaterial;

	if (m_MeshTable.count(filter) == 0)
	{
		DescriptorSetVK* pDescriptorSet = m_pDescriptorPool->allocDescriptorSet(m_pGeometryDescriptorSetLayout);
		pDescriptorSet->writeUniformBufferDescriptor(m_pCameraBuffer, CAMERA_BUFFER_BINDING);

		BufferVK* pVertBuffer = reinterpret_cast<BufferVK*>(pMesh->getVertexBuffer());
		pDescriptorSet->writeStorageBufferDescriptor(pVertBuffer, VERTEX_BUFFER_BINDING);

		SamplerVK* pSampler = reinterpret_cast<SamplerVK*>(pMaterial->getSampler());

		Texture2DVK* pAlbedo = nullptr;
		if (pMaterial->hasAlbedoMap())
		{
			pAlbedo = reinterpret_cast<Texture2DVK*>(pMaterial->getAlbedoMap());
		}
		else
		{
			pAlbedo = m_pDefaultTexture;
		}
		pDescriptorSet->writeCombinedImageDescriptor(pAlbedo->getImageView(), pSampler, ALBEDO_MAP_BINDING);

		Texture2DVK* pNormal = nullptr;
		if (pMaterial->hasNormalMap())
		{
			pNormal = reinterpret_cast<Texture2DVK*>(pMaterial->getNormalMap());
		}
		else
		{
			pNormal = m_pDefaultNormal;
		}
		pDescriptorSet->writeCombinedImageDescriptor(pNormal->getImageView(), pSampler, NORMAL_MAP_BINDING);

		Texture2DVK* pAO = nullptr;
		if (pMaterial->hasAmbientOcclusionMap())
		{
			pAO = reinterpret_cast<Texture2DVK*>(pMaterial->getAmbientOcclusionMap());
		}
		else
		{
			pAO = m_pDefaultTexture;
		}
		pDescriptorSet->writeCombinedImageDescriptor(pAO->getImageView(), pSampler, AO_MAP_BINDING);

		Texture2DVK* pMetallic = nullptr;
		if (pMaterial->hasMetallicMap())
		{
			pMetallic = reinterpret_cast<Texture2DVK*>(pMaterial->getMetallicMap());
		}
		else
		{
			pMetallic = m_pDefaultTexture;
		}
		pDescriptorSet->writeCombinedImageDescriptor(pMetallic->getImageView(), pSampler, METALLIC_MAP_BINDING);

		Texture2DVK* pRoughness = nullptr;
		if (pMaterial->hasRoughnessMap())
		{
			pRoughness = reinterpret_cast<Texture2DVK*>(pMaterial->getRoughnessMap());
		}
		else
		{
			pRoughness = m_pDefaultTexture;
		}
		pDescriptorSet->writeCombinedImageDescriptor(pRoughness->getImageView(), pSampler, ROUGHNESS_MAP_BINDING);

		MeshPipeline meshPipeline = {};
		meshPipeline.pDescriptorSets = pDescriptorSet;

		m_MeshTable.insert(std::make_pair(filter, meshPipeline));
		return pDescriptorSet;
	}

	MeshPipeline meshPipeline = m_MeshTable[filter];
	return meshPipeline.pDescriptorSets;
}
