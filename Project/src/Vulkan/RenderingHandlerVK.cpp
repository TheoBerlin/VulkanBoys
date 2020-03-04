#include "RenderingHandlerVK.h"

#include "Common/IBuffer.h"
#include "Common/IGraphicsContext.h"
#include "Common/IImgui.h"
#include "Common/IRenderer.h"
#include "Common/ParticleEmitterHandler.h"

#include "BufferVK.h"
#include "CommandBufferVK.h"
#include "CommandPoolVK.h"
#include "FrameBufferVK.h"
#include "GraphicsContextVK.h"
#include "MeshRendererVK.h"
#include "PipelineVK.h"
#include "RenderPassVK.h"
#include "SwapChainVK.h"
#include "TextureCubeVK.h"
#include "SkyboxRendererVK.h"

#include "Particles/ParticleEmitterHandlerVK.h"
#include "Particles/ParticleRendererVK.h"

#include "Ray Tracing/RayTracingRendererVK.h"

RenderingHandlerVK::RenderingHandlerVK(GraphicsContextVK* pGraphicsContext)
    :m_pGraphicsContext(pGraphicsContext),
	m_pMeshRenderer(nullptr),
	m_pRayTracer(nullptr),
	m_pParticleRenderer(nullptr),
    m_pRenderPass(nullptr),
	m_pCameraDirectionsBuffer(nullptr),
    m_CurrentFrame(0),
	m_BackBufferIndex(0),
	m_ClearColor(),
	m_ClearDepth(),
	m_Viewport(),
	m_ScissorRect()
{
	m_ClearDepth.depthStencil.depth = 1.0f;
	m_ClearDepth.depthStencil.stencil = 0;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
        m_ppGraphicsCommandPools[i]		= VK_NULL_HANDLE;
		m_ppComputeCommandPools[i]		= VK_NULL_HANDLE;
        m_ppGraphicsCommandBuffers[i]	= VK_NULL_HANDLE;
    }

	m_EnableRayTracing = m_pGraphicsContext->supportsRayTracing();
}

RenderingHandlerVK::~RenderingHandlerVK()
{
	SAFEDELETE(m_pCameraMatricesBuffer);
	SAFEDELETE(m_pCameraDirectionsBuffer);
	SAFEDELETE(m_pRenderPass);
	SAFEDELETE(m_pSkyboxRenderer);
	releaseBackBuffers();

	VkDevice device = m_pGraphicsContext->getDevice()->getDevice();
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		SAFEDELETE(m_ppGraphicsCommandPools[i]);
		SAFEDELETE(m_ppComputeCommandPools[i]);
		SAFEDELETE(m_ppCommandPoolsSecondary[i]);

		if (m_ImageAvailableSemaphores[i] != VK_NULL_HANDLE) 
		{
			vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
		}

		if (m_RenderFinishedSemaphores[i] != VK_NULL_HANDLE) 
		{
			vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
		}
    }
}

bool RenderingHandlerVK::initialize()
{
	m_pSkyboxRenderer = DBG_NEW SkyboxRendererVK(m_pGraphicsContext->getDevice());
	if (!m_pSkyboxRenderer->init())
	{
		return false;
	}

	if (!createRenderPass()) 
	{
		return false;
	}

	if (!createBackBuffers()) 
	{
		return false;
	}

	if (!createCommandPoolAndBuffers()) 
	{
		return false;
	}

	if (!createSemaphores()) 
	{
		return false;
	}

	if (!createBuffers()) 
	{
		return false;
	}

	return true;
}

ITextureCube* RenderingHandlerVK::generateTextureCube(ITexture2D* pPanorama, ETextureFormat format, uint32_t width, uint32_t miplevels)
{
	TextureCubeVK* pTextureCube = DBG_NEW TextureCubeVK(m_pGraphicsContext->getDevice());
	pTextureCube->init(width, miplevels, format);

	Texture2DVK* pPanoramaVK = reinterpret_cast<Texture2DVK*>(pPanorama);
	m_pSkyboxRenderer->generateCubemapFromPanorama(pTextureCube, pPanoramaVK);

	return pTextureCube;
}

void RenderingHandlerVK::onWindowResize(uint32_t width, uint32_t height)
{
	m_pGraphicsContext->getDevice()->wait();
	releaseBackBuffers();

	m_pGraphicsContext->getSwapChain()->resize(width, height);

	m_pMeshRenderer->onWindowResize(width, height);

	createBackBuffers();
}

void RenderingHandlerVK::beginFrame(const Camera& camera, const LightSetup& lightsetup)
{
	SwapChainVK* pSwapChain = m_pGraphicsContext->getSwapChain();
	pSwapChain->acquireNextImage(m_ImageAvailableSemaphores[m_CurrentFrame]);
	m_BackBufferIndex = pSwapChain->getImageIndex();

	// Prepare for frame
	m_ppGraphicsCommandBuffers[m_CurrentFrame]->reset(true);
	m_ppGraphicsCommandPools[m_CurrentFrame]->reset();
	m_ppGraphicsCommandBuffers[m_CurrentFrame]->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	m_ppComputeCommandBuffers[m_CurrentFrame]->reset(true);
	m_ppComputeCommandPools[m_CurrentFrame]->reset();
	m_ppComputeCommandBuffers[m_CurrentFrame]->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	//m_ppCommandBuffersSecondary[m_CurrentFrame]->reset(false);
	//m_ppCommandPoolsSecondary[m_CurrentFrame]->reset();

	// Needed to begin a secondary buffer
	//VkCommandBufferInheritanceInfo inheritanceInfo = {};
	//inheritanceInfo.sType		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	//inheritanceInfo.pNext		= nullptr;
	//inheritanceInfo.renderPass	= m_pRenderPass->getRenderPass();
	//inheritanceInfo.subpass		= 0; // TODO: Don't hardcode this :(
	//inheritanceInfo.framebuffer = m_ppBackbuffers[m_BackBufferIndex]->getFrameBuffer();
	//m_ppCommandBuffersSecondary[m_CurrentFrame]->begin(&inheritanceInfo, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	updateBuffers(camera);

	//if (m_pRayTracer != nullptr) 
	//{
	//	m_pRayTracer->beginFrame(camera, lightsetup);
	//} 
	
	if (m_pMeshRenderer != nullptr) 
	{
		m_pMeshRenderer->beginFrame(camera, lightsetup);
	}

/*	if (m_pParticleRenderer != nullptr) 
	{
		m_pParticleRenderer->beginFrame(camera, lightsetup);
		submitParticles();
	}	*/	

	//startRenderPass();
}

void RenderingHandlerVK::endFrame()
{
	//if (m_pRayTracer != nullptr) 
	//{
	//	m_pRayTracer->endFrame();
	//}

	if (m_pMeshRenderer != nullptr) 
	{
		m_pMeshRenderer->endFrame();
	}

	//if (m_pParticleRenderer != nullptr) 
	//{
	//	m_pParticleRenderer->endFrame();
	//}

    // Submit the rendering handler's command buffer
	DeviceVK* pDevice = m_pGraphicsContext->getDevice();

	if (m_pRayTracer == nullptr) 
	{
		// Execute renderers' secondary command buffers
		//m_ppCommandBuffersSecondary[m_CurrentFrame]->end();
		//pDevice->executeSecondaryCommandBuffer(m_ppGraphicsCommandBuffers[m_CurrentFrame], m_ppCommandBuffersSecondary[m_CurrentFrame]);

  //  	m_ppGraphicsCommandBuffers[m_CurrentFrame]->endRenderPass();

		//ParticleEmitterHandlerVK* pEmitterHandler = reinterpret_cast<ParticleEmitterHandlerVK*>(m_pParticleEmitterHandler);
		//if (pEmitterHandler->gpuComputed()) 
		//{
		//	for (ParticleEmitter* pEmitter : pEmitterHandler->getParticleEmitters()) 
		//	{
		//		pEmitterHandler->releaseFromGraphics(reinterpret_cast<BufferVK*>(pEmitter->getPositionsBuffer()), m_ppGraphicsCommandBuffers[m_CurrentFrame]);
		//	}
		//}
	}

	if (m_pMeshRenderer)
	{
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->executeSecondary(m_pMeshRenderer->getCurrentCommandBuffer());
	}

	m_ppGraphicsCommandBuffers[m_CurrentFrame]->end();
	m_ppComputeCommandBuffers[m_CurrentFrame]->end();

	// Execute commandbuffer
	VkSemaphore waitSemaphores[]		= { m_ImageAvailableSemaphores[m_CurrentFrame] };
	VkSemaphore signalSemaphores[]		= { m_RenderFinishedSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags waitStages[]	= { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	//pDevice->executeCommandBuffer(pDevice->getComputeQueue(), m_ppComputeCommandBuffers[m_CurrentFrame], nullptr, nullptr, 0, nullptr, 0);
	pDevice->executeCommandBuffer(pDevice->getGraphicsQueue(), m_ppGraphicsCommandBuffers[m_CurrentFrame], waitSemaphores, waitStages, 1, signalSemaphores, 1);

	// Write profiler results
	//if (m_pMeshRenderer)
	//{
	//	ProfilerVK* pMeshProfiler = m_pMeshRenderer->getProfiler();
	//	pMeshProfiler->writeResults();
	//} 

	//if (m_pRayTracer)
	//{
	//	ProfilerVK* pRTProfiler = m_pRayTracer->getProfiler();
	//	pRTProfiler->writeResults();
	//}
}

void RenderingHandlerVK::swapBuffers()
{
    m_pGraphicsContext->swapBuffers(m_RenderFinishedSemaphores[m_CurrentFrame]);
	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RenderingHandlerVK::drawProfilerUI()
{
	if (m_pMeshRenderer)
	{
		ProfilerVK* pMeshProfiler = m_pMeshRenderer->getProfiler();
		pMeshProfiler->drawResults();
	} 
	
	if (m_pRayTracer)
	{
		ProfilerVK* pRTProfiler = m_pRayTracer->getProfiler();
		pRTProfiler->drawResults();
	}
}

void RenderingHandlerVK::drawImgui(IImgui* pImgui)
{
    pImgui->render(m_ppCommandBuffersSecondary[m_CurrentFrame]);
}

void RenderingHandlerVK::setClearColor(float r, float g, float b)
{
	setClearColor(glm::vec3(r, g, b));
}

void RenderingHandlerVK::setClearColor(const glm::vec3& color)
{
	m_ClearColor.color.float32[0] = color.r;
	m_ClearColor.color.float32[1] = color.g;
	m_ClearColor.color.float32[2] = color.b;
	m_ClearColor.color.float32[3] = 1.0f;
}

void RenderingHandlerVK::setViewport(float width, float height, float minDepth, float maxDepth, float topX, float topY)
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

	if (m_pMeshRenderer)
	{
		m_pMeshRenderer->setViewport(width, height, minDepth, maxDepth, topX, topY);
	}

	if (m_pParticleRenderer)
	{
		m_pParticleRenderer->setViewport(width, height, minDepth, maxDepth, topX, topY);
	}
}

void RenderingHandlerVK::setSkybox(ITextureCube* pSkybox)
{
	TextureCubeVK* pSkyboxVK = reinterpret_cast<TextureCubeVK*>(pSkybox);

	//Generate irradiance from the newly set skybox
	TextureCubeVK* pIrradianceMap = DBG_NEW TextureCubeVK(m_pGraphicsContext->getDevice());
	if (pIrradianceMap->init(32, 1, ETextureFormat::FORMAT_R16G16B16A16_FLOAT))
	{
		m_pSkyboxRenderer->generateIrradiance(pSkyboxVK, pIrradianceMap);
	}

	constexpr uint32_t size = 128;
	const uint32_t miplevels = 5; //std::floor(std::log2(size)) + 1U;;

	//Generate pre filtered environmentmap
	TextureCubeVK* pEnvironmentMap = DBG_NEW TextureCubeVK(m_pGraphicsContext->getDevice());
	if (pEnvironmentMap->init(size, miplevels, ETextureFormat::FORMAT_R16G16B16A16_FLOAT))
	{
		m_pSkyboxRenderer->prefilterEnvironmentMap(pSkyboxVK, pEnvironmentMap);
	}

	m_pMeshRenderer->setSkybox(reinterpret_cast<TextureCubeVK*>(pSkybox), pIrradianceMap, pEnvironmentMap);
}

void RenderingHandlerVK::submitMesh(IMesh* pMesh, const Material& material, const glm::mat4& transform)
{
	m_pMeshRenderer->submitMesh(pMesh, material, transform);
}

bool RenderingHandlerVK::createBackBuffers()
{
    SwapChainVK* pSwapChain = m_pGraphicsContext->getSwapChain();
	DeviceVK* pDevice = m_pGraphicsContext->getDevice();

	VkExtent2D extent = pSwapChain->getExtent();
	ImageViewVK* pDepthStencilView = pSwapChain->getDepthStencilView();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		m_ppBackbuffers[i] = DBG_NEW FrameBufferVK(pDevice);
		m_ppBackbuffers[i]->addColorAttachment(pSwapChain->getImageView(i));
		m_ppBackbuffers[i]->setDepthStencilAttachment(pDepthStencilView);
		
		if (!m_ppBackbuffers[i]->finalize(m_pRenderPass, extent.width, extent.height)) 
		{
			return false;
		}
	}

	return true;
}

bool RenderingHandlerVK::createCommandPoolAndBuffers()
{
    DeviceVK* pDevice = m_pGraphicsContext->getDevice();
    const uint32_t graphicsQueueFamilyIndex = pDevice->getQueueFamilyIndices().graphicsFamily.value();
	const uint32_t gcomputeQueueFamilyIndex = pDevice->getQueueFamilyIndices().computeFamily.value();

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
        m_ppGraphicsCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, graphicsQueueFamilyIndex);
		if (!m_ppGraphicsCommandPools[i]->init()) 
		{
			return false;
		}

        m_ppGraphicsCommandBuffers[i] = m_ppGraphicsCommandPools[i]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        if (m_ppGraphicsCommandBuffers[i] == nullptr) 
		{
            return false;
        }

		m_ppComputeCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, gcomputeQueueFamilyIndex);
		if (!m_ppComputeCommandPools[i]->init()) 
		{
			return false;
		}

		m_ppComputeCommandBuffers[i] = m_ppComputeCommandPools[i]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		if (m_ppComputeCommandBuffers[i] == nullptr) 
		{
			return false;
		}

        m_ppCommandPoolsSecondary[i] = DBG_NEW CommandPoolVK(pDevice, graphicsQueueFamilyIndex);
		if (!m_ppCommandPoolsSecondary[i]->init()) 
		{
			return false;
		}

        m_ppCommandBuffersSecondary[i] = m_ppCommandPoolsSecondary[i]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
        if (m_ppCommandBuffersSecondary[i] == nullptr) 
		{
            return false;
        }
    }

	return true;
}

bool RenderingHandlerVK::createRenderPass()
{
	//Create renderpass
	m_pRenderPass = DBG_NEW RenderPassVK(m_pGraphicsContext->getDevice());
	VkAttachmentDescription description = {};
	description.format			= VK_FORMAT_B8G8R8A8_UNORM;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;			
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;			
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;	
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;	
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;			
	description.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		
	m_pRenderPass->addAttachment(description);

	description.format			= VK_FORMAT_D24_UNORM_S8_UINT;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;				
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;				
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;	
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;	
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;							
	description.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;		
	m_pRenderPass->addAttachment(description);

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment	= 0;
	colorAttachmentRef.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentRef = {};
	depthStencilAttachmentRef.attachment	= 1;
	depthStencilAttachmentRef.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pRenderPass->addSubpass(&colorAttachmentRef, 1, &depthStencilAttachmentRef);

	VkSubpassDependency dependency = {};
	dependency.srcSubpass		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass		= 0;
	dependency.dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;
	dependency.srcAccessMask	= 0;
	dependency.dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	m_pRenderPass->addSubpassDependency(dependency);

	return m_pRenderPass->finalize();
}

bool RenderingHandlerVK::createSemaphores()
{
    // Create semaphores
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	VkDevice device = m_pGraphicsContext->getDevice()->getDevice();
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]), "Failed to create semaphores for a Frame");
		VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]), "Failed to create semaphores for a Frame");
	}

	return true;
}

void RenderingHandlerVK::releaseBackBuffers()
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		SAFEDELETE(m_ppBackbuffers[i]);
	}
}

void RenderingHandlerVK::updateBuffers(const Camera& camera)
{
	// Update camera buffers
	CameraMatricesBuffer cameraMatricesBuffer = {};
	cameraMatricesBuffer.Projection = camera.getProjectionMat();
	cameraMatricesBuffer.View		= camera.getViewMat();
	m_ppGraphicsCommandBuffers[m_CurrentFrame]->updateBuffer(m_pCameraMatricesBuffer, 0, (const void*)&cameraMatricesBuffer, sizeof(CameraMatricesBuffer));

	CameraDirectionsBuffer cameraDirectionsBuffer = {};
	cameraDirectionsBuffer.Right	= glm::vec4(camera.getRightVec(), 0.0f);
	cameraDirectionsBuffer.Up		= glm::vec4(camera.getUpVec(), 0.0f);
	m_ppGraphicsCommandBuffers[m_CurrentFrame]->updateBuffer(m_pCameraDirectionsBuffer, 0, (const void*)&cameraDirectionsBuffer, sizeof(CameraDirectionsBuffer));

	// Update particle buffers
	//m_pParticleEmitterHandler->updateRenderingBuffers(this);
}

void RenderingHandlerVK::startRenderPass()
{
	// Start renderpass
	VkClearValue clearValues[] = { m_ClearColor, m_ClearDepth };
	m_ppGraphicsCommandBuffers[m_CurrentFrame]->beginRenderPass(m_pRenderPass, m_ppBackbuffers[m_BackBufferIndex], (uint32_t)m_Viewport.width, (uint32_t)m_Viewport.height, clearValues, 2, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

void RenderingHandlerVK::submitParticles()
{
	ParticleEmitterHandlerVK* pEmitterHandler = reinterpret_cast<ParticleEmitterHandlerVK*>(m_pParticleEmitterHandler);

	// Transfer buffer ownerships to the graphics queue
	for (ParticleEmitter* pEmitter : pEmitterHandler->getParticleEmitters()) 
	{
		if (pEmitterHandler->gpuComputed()) 
		{
			pEmitterHandler->acquireForGraphics(reinterpret_cast<BufferVK*>(pEmitter->getPositionsBuffer()), m_ppGraphicsCommandBuffers[m_CurrentFrame]);
		}

		m_pParticleRenderer->submitParticles(pEmitter);
	}
}

bool RenderingHandlerVK::createBuffers()
{
	// Create camera matrices buffer
	BufferParams cameraBufferParams = {};
	cameraBufferParams.Usage			= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	cameraBufferParams.SizeInBytes		= sizeof(CameraMatricesBuffer);
	cameraBufferParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	m_pCameraMatricesBuffer = DBG_NEW BufferVK(m_pGraphicsContext->getDevice());
	if (!m_pCameraMatricesBuffer->init(cameraBufferParams)) 
	{
		LOG("Failed to create camera matrices buffer");
		return false;
	}

	// Create camera directions buffer
	cameraBufferParams.SizeInBytes = sizeof(CameraDirectionsBuffer);

	m_pCameraDirectionsBuffer = DBG_NEW BufferVK(m_pGraphicsContext->getDevice());
	if (!m_pCameraDirectionsBuffer->init(cameraBufferParams)) 
	{
		LOG("Failed to create camera directions buffer");
		return false;
	}

	return true;
}
