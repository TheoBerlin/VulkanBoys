#include "RenderingHandlerVK.h"

#include "Common/IBuffer.h"
#include "Common/IGraphicsContext.h"
#include "Common/IImgui.h"
#include "Common/IRenderer.h"
#include "Common/ParticleEmitterHandler.h"

#include "Core/TaskDispatcher.h"

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
#include "ImguiVK.h"
#include "GBufferVK.h"
#include "SceneVK.h"

#include "Particles/ParticleEmitterHandlerVK.h"
#include "Particles/ParticleRendererVK.h"

#include "Ray Tracing/RayTracingRendererVK.h"
#include <Vulkan\ImageVK.h>
#include <Vulkan\ImageViewVK.h>

constexpr uint32_t RAY_TRACING_RESOLUTION_DENOMINATOR = 1;

RenderingHandlerVK::RenderingHandlerVK(GraphicsContextVK* pGraphicsContext)
	:m_pGraphicsContext(pGraphicsContext),
	m_pMeshRenderer(nullptr),
	m_pRayTracer(nullptr),
	m_pParticleRenderer(nullptr),
	m_pRayTracingStorageImage(nullptr),
	m_pRayTracingStorageImageView(nullptr),
	m_pGBuffer(nullptr),
	m_pGeometryRenderPass(nullptr),
	m_pBackBufferRenderPass(nullptr),
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
}

RenderingHandlerVK::~RenderingHandlerVK()
{
	SAFEDELETE(m_pCameraMatricesBuffer);
	SAFEDELETE(m_pCameraDirectionsBuffer);
	SAFEDELETE(m_pGeometryRenderPass);
	SAFEDELETE(m_pBackBufferRenderPass);
	SAFEDELETE(m_pSkyboxRenderer);
	SAFEDELETE(m_pRayTracingStorageImage);
	SAFEDELETE(m_pRayTracingStorageImageView);
	SAFEDELETE(m_pGBuffer);
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

	if (!createRenderPasses()) 
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

	if (!createRayTracingRenderImage(m_pGraphicsContext->getSwapChain()->getExtent().width, m_pGraphicsContext->getSwapChain()->getExtent().height))
	{
		return false;
	}

	if (!createGBuffer())
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

void RenderingHandlerVK::render(IScene* pScene)
{
	SceneVK* pVulkanScene = reinterpret_cast<SceneVK*>(pScene);

	//Todo: These should probably not be seperate anymore
	beginFrame(pVulkanScene);
	endFrame(pVulkanScene);
	swapBuffers();
}

void RenderingHandlerVK::onWindowResize(uint32_t width, uint32_t height)
{
	m_pGraphicsContext->getDevice()->wait();
	releaseBackBuffers();

	m_pGraphicsContext->getSwapChain()->resize(width, height);

	m_pMeshRenderer->onWindowResize(width, height);

	if (m_pRayTracer != nullptr)
	{
		m_pRayTracer->onWindowResize(width, height);
		//Temp?
		m_pRayTracer->setResolution(width / RAY_TRACING_RESOLUTION_DENOMINATOR, height / RAY_TRACING_RESOLUTION_DENOMINATOR);
	}

	createRayTracingRenderImage(width, height);

	createBackBuffers();
}

void RenderingHandlerVK::beginFrame(SceneVK* pScene)
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

	const Camera& camera = pScene->getCamera();
	const LightSetup& lightsetup = pScene->getLightSetup();

	updateBuffers(camera);

	if (m_pMeshRenderer != nullptr)
	{
		m_pMeshRenderer->setupFrame(m_ppGraphicsCommandBuffers[m_CurrentFrame], camera, lightsetup);
		m_pMeshRenderer->beginFrame(pScene);
	}

	if (m_pParticleRenderer != nullptr)
	{
		m_pParticleRenderer->beginFrame(pScene);
		submitParticles();
	}
	
	//startRenderPass();
}

void RenderingHandlerVK::endFrame(SceneVK* pScene)
{
	DeviceVK* pDevice = m_pGraphicsContext->getDevice();

	// Submit the rendering handler's command buffer
	if (m_pParticleEmitterHandler)
	{
		ParticleEmitterHandlerVK* pEmitterHandler = reinterpret_cast<ParticleEmitterHandlerVK*>(m_pParticleEmitterHandler);
		if (pEmitterHandler->gpuComputed())
		{
			for (ParticleEmitter* pEmitter : pEmitterHandler->getParticleEmitters())
			{
				pEmitterHandler->releaseFromGraphics(reinterpret_cast<BufferVK*>(pEmitter->getPositionsBuffer()), m_ppGraphicsCommandBuffers[m_CurrentFrame]);
			}
		}
	}

	//Render all the meshes
	FrameBufferVK* pBackbuffer = getCurrentBackBuffer();
	CommandBufferVK* pSecondaryCommandBuffer = m_ppCommandBuffersSecondary[m_CurrentFrame];
	CommandPoolVK* pSecondaryCommandPool = m_ppCommandPoolsSecondary[m_CurrentFrame];

	if (m_pImGuiRenderer)
	{
		TaskDispatcher::execute([&, this]
			{
				// Needed to begin a secondary buffer
				VkCommandBufferInheritanceInfo inheritanceInfo = {};
				inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
				inheritanceInfo.pNext = nullptr;
				inheritanceInfo.renderPass = m_pBackBufferRenderPass->getRenderPass();
				inheritanceInfo.subpass = 0; // TODO: Don't hardcode this :(
				inheritanceInfo.framebuffer = pBackbuffer->getFrameBuffer();

				pSecondaryCommandBuffer->reset(false);
				pSecondaryCommandPool->reset();
				pSecondaryCommandBuffer->begin(&inheritanceInfo, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);

				m_pImGuiRenderer->render(pSecondaryCommandBuffer);
				pSecondaryCommandBuffer->end();
			});
	}

	if (m_pMeshRenderer)
	{
		//Start renderpass
		VkClearValue clearValues[] = { m_ClearColor, m_ClearColor, m_ClearColor, m_ClearDepth };
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->beginRenderPass(m_pGeometryRenderPass, m_pGBuffer->getFrameBuffer(), (uint32_t)m_Viewport.width, (uint32_t)m_Viewport.height, clearValues, 4, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		TaskDispatcher::execute([pScene, this]
			{

				for (auto& graphicsObject : pScene->getGraphicsObjects())
				{
					m_pMeshRenderer->submitMesh(graphicsObject.pMesh, graphicsObject.pMaterial, graphicsObject.Transform);
				}
				m_pMeshRenderer->endFrame(pScene);
			});

		TaskDispatcher::execute([this]
			{
				m_pMeshRenderer->setRayTracingResult(m_pRayTracingStorageImageView);
				m_pMeshRenderer->buildLightPass(m_pBackBufferRenderPass, getCurrentBackBuffer());
			});

		TaskDispatcher::waitForTasks();
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->executeSecondary(m_pMeshRenderer->getGeometryCommandBuffer());
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->endRenderPass();

		m_ppGraphicsCommandBuffers[m_CurrentFrame]->releaseImagesOwnership(
			m_pGBuffer->getColorImages(),
			m_pGBuffer->getColorImageCount(),
			VK_ACCESS_MEMORY_READ_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_SHADER_STAGE_RAYGEN_BIT_NV);

		m_ppGraphicsCommandBuffers[m_CurrentFrame]->releaseImageOwnership(
			m_pGBuffer->getDepthImage(),
			VK_ACCESS_MEMORY_READ_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_SHADER_STAGE_RAYGEN_BIT_NV,
			VK_IMAGE_ASPECT_DEPTH_BIT);

		m_ppGraphicsCommandBuffers[m_CurrentFrame]->releaseImageOwnership(
			m_pRayTracingStorageImage,
			VK_ACCESS_MEMORY_WRITE_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_SHADER_STAGE_RAYGEN_BIT_NV);

		m_ppGraphicsCommandBuffers[m_CurrentFrame]->end();
		pDevice->executeCommandBuffer(pDevice->getGraphicsQueue(), m_ppGraphicsCommandBuffers[m_CurrentFrame], nullptr, nullptr, 0, nullptr, 0);
		m_pGraphicsContext->getDevice()->wait();
	}

	//Ray Tracing
	if (m_pRayTracer != nullptr)
	{
		m_ppComputeCommandBuffers[m_CurrentFrame]->acquireImagesOwnership(
			m_pGBuffer->getColorImages(),
			m_pGBuffer->getColorImageCount(),
			VK_ACCESS_MEMORY_READ_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_SHADER_STAGE_RAYGEN_BIT_NV);


		m_ppComputeCommandBuffers[m_CurrentFrame]->acquireImageOwnership(
			m_pGBuffer->getDepthImage(),
			VK_ACCESS_MEMORY_READ_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_SHADER_STAGE_RAYGEN_BIT_NV,
			VK_IMAGE_ASPECT_DEPTH_BIT);

		//Todo: Combine this and acquire to same
		m_ppComputeCommandBuffers[m_CurrentFrame]->transitionImageLayout(m_pGBuffer->getDepthImage(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1, VK_IMAGE_ASPECT_DEPTH_BIT);


		m_ppComputeCommandBuffers[m_CurrentFrame]->acquireImageOwnership(
			m_pRayTracingStorageImage,
			VK_ACCESS_MEMORY_WRITE_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_SHADER_STAGE_RAYGEN_BIT_NV);

		//Todo: Combine this and acquire to same
		m_ppComputeCommandBuffers[m_CurrentFrame]->transitionImageLayout(m_pRayTracingStorageImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1);

		m_pRayTracer->setRayTracingResult(m_pRayTracingStorageImageView, m_pGraphicsContext->getSwapChain()->getExtent().width, m_pGraphicsContext->getSwapChain()->getExtent().height);
		m_pRayTracer->render(pScene, m_pGBuffer);
		m_ppComputeCommandBuffers[m_CurrentFrame]->executeSecondary(m_pRayTracer->getComputeCommandBuffer());

		m_ppComputeCommandBuffers[m_CurrentFrame]->releaseImagesOwnership(
			m_pGBuffer->getColorImages(),
			m_pGBuffer->getColorImageCount(),
			VK_ACCESS_MEMORY_READ_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			VK_SHADER_STAGE_RAYGEN_BIT_NV,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		//Todo: Combine this and release to same
		m_ppComputeCommandBuffers[m_CurrentFrame]->transitionImageLayout(m_pGBuffer->getDepthImage(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0, 1, 0, 1, VK_IMAGE_ASPECT_DEPTH_BIT);

		m_ppComputeCommandBuffers[m_CurrentFrame]->releaseImageOwnership(
			m_pGBuffer->getDepthImage(),
			VK_ACCESS_MEMORY_READ_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			VK_SHADER_STAGE_RAYGEN_BIT_NV,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT);

		//Todo: Combine this and release to same
		m_ppComputeCommandBuffers[m_CurrentFrame]->transitionImageLayout(m_pRayTracingStorageImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);

		m_ppComputeCommandBuffers[m_CurrentFrame]->releaseImageOwnership(
			m_pRayTracingStorageImage,
			VK_ACCESS_MEMORY_READ_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			VK_SHADER_STAGE_RAYGEN_BIT_NV,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}

	if (m_pMeshRenderer)
	{
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->reset(true);
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->acquireImagesOwnership(
			m_pGBuffer->getColorImages(),
			m_pGBuffer->getColorImageCount(),
			VK_ACCESS_MEMORY_READ_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			VK_SHADER_STAGE_RAYGEN_BIT_NV,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		m_ppGraphicsCommandBuffers[m_CurrentFrame]->acquireImageOwnership(
			m_pGBuffer->getDepthImage(),
			VK_ACCESS_MEMORY_READ_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			VK_SHADER_STAGE_RAYGEN_BIT_NV,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT);

		m_ppGraphicsCommandBuffers[m_CurrentFrame]->acquireImageOwnership(
			m_pRayTracingStorageImage,
			VK_ACCESS_MEMORY_READ_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			VK_SHADER_STAGE_RAYGEN_BIT_NV,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}

	//Gather all renderer's data and finalize the frame
	VkClearValue clearValues[] = { m_ClearColor, m_ClearDepth };
	m_ppGraphicsCommandBuffers[m_CurrentFrame]->beginRenderPass(m_pBackBufferRenderPass, pBackbuffer, (uint32_t)m_Viewport.width, (uint32_t)m_Viewport.height, clearValues, 2, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	if (m_pMeshRenderer)
	{
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->executeSecondary(m_pMeshRenderer->getLightCommandBuffer());
	}

	if (m_pParticleRenderer)
	{
		m_pParticleRenderer->endFrame(pScene);
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->executeSecondary(m_pParticleRenderer->getCommandBuffer(m_CurrentFrame));
	}

	m_ppGraphicsCommandBuffers[m_CurrentFrame]->executeSecondary(pSecondaryCommandBuffer);

	m_ppGraphicsCommandBuffers[m_CurrentFrame]->endRenderPass();

	m_ppGraphicsCommandBuffers[m_CurrentFrame]->end();
	m_ppComputeCommandBuffers[m_CurrentFrame]->end();

	// Execute commandbuffer
	VkSemaphore graphicsWaitSemaphores[]		= { m_ImageAvailableSemaphores[m_CurrentFrame] };
	VkSemaphore graphicsSignalSemaphores[]		= { m_RenderFinishedSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags graphicswaitStages[]	= { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	pDevice->executeCommandBuffer(pDevice->getComputeQueue(), m_ppComputeCommandBuffers[m_CurrentFrame], nullptr, nullptr, 0, nullptr, 0);
	m_pGraphicsContext->getDevice()->wait();
	pDevice->executeCommandBuffer(pDevice->getGraphicsQueue(), m_ppGraphicsCommandBuffers[m_CurrentFrame], graphicsWaitSemaphores, graphicswaitStages, 1, graphicsSignalSemaphores, 1);
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

	if (m_pRayTracer != nullptr)
	{
		m_pRayTracer->setSkybox(reinterpret_cast<TextureCubeVK*>(pSkybox));
	}
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
		
		if (!m_ppBackbuffers[i]->finalize(m_pBackBufferRenderPass, extent.width, extent.height)) 
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

bool RenderingHandlerVK::createRenderPasses()
{
	//Create Backbuffer Renderpass
	m_pBackBufferRenderPass = DBG_NEW RenderPassVK(m_pGraphicsContext->getDevice());
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
	dependency.srcSubpass		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass		= 0;
	dependency.dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;
	dependency.srcAccessMask	= 0;
	dependency.dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	m_pBackBufferRenderPass->addSubpassDependency(dependency);

	if (!m_pBackBufferRenderPass->finalize())
	{
		return false;
	}

	//Create Geometry Renderpass
	m_pGeometryRenderPass = DBG_NEW RenderPassVK(m_pGraphicsContext->getDevice());

	//Albedo
	description.format = VK_FORMAT_R8G8B8A8_UNORM;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_pGeometryRenderPass->addAttachment(description);

	//Normals
	description.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_pGeometryRenderPass->addAttachment(description);

	//World position
	description.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_pGeometryRenderPass->addAttachment(description);

	//Depth
	description.format = VK_FORMAT_D32_SFLOAT;// VK_FORMAT_D24_UNORM_S8_UINT;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pGeometryRenderPass->addAttachment(description);

	VkAttachmentReference colorAttachmentRefs[3];
	//Albedo
	colorAttachmentRefs[0].attachment = 0;
	colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//Normals
	colorAttachmentRefs[1].attachment = 1;
	colorAttachmentRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//Positions
	colorAttachmentRefs[2].attachment = 2;
	colorAttachmentRefs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	depthStencilAttachmentRef = {};
	depthStencilAttachmentRef.attachment = 3;
	depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	m_pGeometryRenderPass->addSubpass(colorAttachmentRefs, 3, &depthStencilAttachmentRef);

	dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	m_pGeometryRenderPass->addSubpassDependency(dependency);

	dependency.srcSubpass = 0;
	dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	m_pGeometryRenderPass->addSubpassDependency(dependency);

	return m_pGeometryRenderPass->finalize();
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
	cameraMatricesBuffer.ProjectionInv = camera.getProjectionMat();
	cameraMatricesBuffer.ViewInv		= camera.getViewMat();
	m_ppGraphicsCommandBuffers[m_CurrentFrame]->updateBuffer(m_pCameraMatricesBuffer, 0, (const void*)&cameraMatricesBuffer, sizeof(CameraMatricesBuffer));

	CameraDirectionsBuffer cameraDirectionsBuffer = {};
	cameraDirectionsBuffer.Right	= glm::vec4(camera.getRightVec(), 0.0f);
	cameraDirectionsBuffer.Up		= glm::vec4(camera.getUpVec(), 0.0f);
	m_ppGraphicsCommandBuffers[m_CurrentFrame]->updateBuffer(m_pCameraDirectionsBuffer, 0, (const void*)&cameraDirectionsBuffer, sizeof(CameraDirectionsBuffer));

	// Update particle buffers
	m_pParticleEmitterHandler->updateRenderingBuffers(this);
}

void RenderingHandlerVK::startRenderPass()
{
	// Start renderpass
	VkClearValue clearValues[] = { m_ClearColor, m_ClearDepth };
	m_ppGraphicsCommandBuffers[m_CurrentFrame]->beginRenderPass(m_pBackBufferRenderPass, m_ppBackbuffers[m_BackBufferIndex], (uint32_t)m_Viewport.width, (uint32_t)m_Viewport.height, clearValues, 2, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
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

bool RenderingHandlerVK::createRayTracingRenderImage(uint32_t width, uint32_t height)
{
	SAFEDELETE(m_pRayTracingStorageImage);
	SAFEDELETE(m_pRayTracingStorageImageView);

	ImageParams imageParams = {};
	imageParams.Type = VK_IMAGE_TYPE_2D;
	imageParams.Format = VK_FORMAT_A2B10G10R10_UNORM_PACK32; //Todo: What format should this be?
	imageParams.Extent.width = width / RAY_TRACING_RESOLUTION_DENOMINATOR;
	imageParams.Extent.height = height / RAY_TRACING_RESOLUTION_DENOMINATOR;
	imageParams.Extent.depth = 1;
	imageParams.MipLevels = 1;
	imageParams.ArrayLayers = 1;
	imageParams.Samples = VK_SAMPLE_COUNT_1_BIT;
	imageParams.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	m_pRayTracingStorageImage = new ImageVK(m_pGraphicsContext->getDevice());
	m_pRayTracingStorageImage->init(imageParams);

	ImageViewParams imageViewParams = {};
	imageViewParams.Type = VK_IMAGE_VIEW_TYPE_2D;
	imageViewParams.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewParams.FirstMipLevel = 0;
	imageViewParams.MipLevels = 1;
	imageViewParams.FirstLayer = 0;
	imageViewParams.LayerCount = 1;

	m_pRayTracingStorageImageView = new ImageViewVK(m_pGraphicsContext->getDevice(), m_pRayTracingStorageImage);
	m_pRayTracingStorageImageView->init(imageViewParams);

	CommandBufferVK* pTempCommandBuffer = m_ppComputeCommandPools[0]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	pTempCommandBuffer->reset(true);
	pTempCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	pTempCommandBuffer->transitionImageLayout(m_pRayTracingStorageImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);
	pTempCommandBuffer->end();

	m_pGraphicsContext->getDevice()->executeCommandBuffer(m_pGraphicsContext->getDevice()->getComputeQueue(), pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
	m_pGraphicsContext->getDevice()->wait();

	m_ppComputeCommandPools[0]->freeCommandBuffer(&pTempCommandBuffer);

	return true;
}

bool RenderingHandlerVK::createGBuffer()
{
	VkExtent2D extent = m_pGraphicsContext->getSwapChain()->getExtent();

	m_pGBuffer = DBG_NEW GBufferVK(m_pGraphicsContext->getDevice());
	m_pGBuffer->addColorAttachmentFormat(VK_FORMAT_R8G8B8A8_UNORM);
	m_pGBuffer->addColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
	m_pGBuffer->addColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT);
	//m_pGBuffer->setDepthAttachmentFormat(VK_FORMAT_D24_UNORM_S8_UINT);
	m_pGBuffer->setDepthAttachmentFormat(VK_FORMAT_D32_SFLOAT);
	return m_pGBuffer->finalize(m_pGeometryRenderPass, extent.width, extent.height);
}

