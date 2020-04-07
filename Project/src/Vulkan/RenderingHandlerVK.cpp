#include "Common/IBuffer.h"
#include "Common/IGraphicsContext.h"
#include "Common/IImgui.h"
#include "Common/IRenderer.h"
#include "Common/ParticleEmitterHandler.h"

#include "Core/PointLight.h"
#include "Core/TaskDispatcher.h"

#include "BufferVK.h"
#include "CommandBufferVK.h"
#include "CommandPoolVK.h"
#include "FrameBufferVK.h"
#include "GBufferVK.h"
#include "GraphicsContextVK.h"
#include "ImageViewVK.h"
#include "ImageVK.h"
#include "ImguiVK.h"
#include "MeshRendererVK.h"
#include "PipelineVK.h"
#include "RenderingHandlerVK.h"
#include "RenderPassVK.h"
#include "SceneVK.h"
#include "ShadowMapRendererVK.h"
#include "SkyboxRendererVK.h"
#include "SwapChainVK.h"
#include "TextureCubeVK.h"

#include "Particles/ParticleEmitterHandlerVK.h"
#include "Particles/ParticleRendererVK.h"

#include "Ray Tracing/RayTracingRendererVK.h"

#include "VolumetricLight/VolumetricLightRendererVK.h"

#define MULTITHREADED 1

RenderingHandlerVK::RenderingHandlerVK(GraphicsContextVK* pGraphicsContext)
	:m_pGraphicsContext(pGraphicsContext),
	m_pMeshRenderer(nullptr),
	m_pShadowMapRenderer(nullptr),
	m_pRayTracer(nullptr),
	m_pVolumetricLightRenderer(nullptr),
	m_pParticleRenderer(nullptr),
	m_pImGuiRenderer(nullptr),
	m_pSkyboxRenderer(nullptr),
	m_pRadianceImage(nullptr),
	m_pRadianceImageView(nullptr),
	m_pGlossyImage(nullptr),
	m_pGlossyImageView(nullptr),
	m_pGBuffer(nullptr),
	m_pGeometryRenderPass(nullptr),
	m_pShadowMapRenderPass(nullptr),
	m_pBackBufferRenderPass(nullptr),
	m_pParticleRenderPass(nullptr),
	m_pUIRenderPass(nullptr),
	m_pCameraBufferCompute(nullptr),
	m_pCameraBufferGraphics(nullptr),
	m_pLightBufferCompute(nullptr),
	m_pLightBufferGraphics(nullptr),
	m_pPipeline(nullptr),
	m_ppBackbuffers(),
	m_ppBackBuffersWithDepth(),
	m_ppCommandPoolsSecondary(),
	m_ppCommandBuffersSecondary(),
	m_ppComputeCommandPools(),
	m_ppComputeCommandBuffers(),
	m_ppGraphicsCommandPools(),
	m_ppGraphicsCommandBuffers(),
	m_ppGraphicsCommandBuffers2(),
	m_pImageAvailableSemaphores(),
	m_pRenderFinishedSemaphores(),
	m_ComputeFinishedGraphicsSemaphore(VK_NULL_HANDLE),
	m_ComputeFinishedTransferSemaphore(VK_NULL_HANDLE),
	m_GeometryFinishedSemaphore(VK_NULL_HANDLE),
	m_TransferFinishedGraphicsSemaphore(VK_NULL_HANDLE),
	m_TransferFinishedComputeSemaphore(VK_NULL_HANDLE),
	m_TransferStartSemaphore(VK_NULL_HANDLE),
    m_CurrentFrame(0),
	m_BackBufferIndex(0),
	m_ClearColor(),
	m_ClearDepth(),
	m_Viewport(),
	m_ScissorRect(),
	m_RayTracingResolutionDenominator(1)
{
	m_ClearDepth.depthStencil.depth = 1.0f;
	m_ClearDepth.depthStencil.stencil = 0;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
        m_ppGraphicsCommandPools[i]		= nullptr;
		m_ppComputeCommandPools[i]		= nullptr;
        m_ppGraphicsCommandBuffers[i]	= nullptr;
		m_ppGraphicsCommandBuffers2[i]	= nullptr;
    }
}

RenderingHandlerVK::~RenderingHandlerVK()
{
	SAFEDELETE(m_pCameraBufferCompute);
	SAFEDELETE(m_pCameraBufferGraphics);

	SAFEDELETE(m_pLightBufferCompute);
	SAFEDELETE(m_pLightBufferGraphics);

	SAFEDELETE(m_pGeometryRenderPass);
	SAFEDELETE(m_pShadowMapRenderPass);
	SAFEDELETE(m_pBackBufferRenderPass);
	SAFEDELETE(m_pParticleRenderPass);
	SAFEDELETE(m_pUIRenderPass);

	SAFEDELETE(m_pSkyboxRenderer);

	SAFEDELETE(m_pRadianceImage);
	SAFEDELETE(m_pRadianceImageView);
	SAFEDELETE(m_pGlossyImage);
	SAFEDELETE(m_pGlossyImageView);

	SAFEDELETE(m_pGBuffer);
	releaseBackBuffers();

	VkDevice device = m_pGraphicsContext->getDevice()->getDevice();
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		SAFEDELETE(m_ppGraphicsCommandPools[i]);
		SAFEDELETE(m_ppTransferCommandPools[i]);
		SAFEDELETE(m_ppComputeCommandPools[i]);
		SAFEDELETE(m_ppCommandPoolsSecondary[i]);

		if (m_pImageAvailableSemaphores[i] != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(device, m_pImageAvailableSemaphores[i], nullptr);
		}

		if (m_pRenderFinishedSemaphores[i] != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(device, m_pRenderFinishedSemaphores[i], nullptr);
		}
    }

	if (m_ComputeFinishedGraphicsSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(device, m_ComputeFinishedGraphicsSemaphore, nullptr);
	}

	if (m_ComputeFinishedTransferSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(device, m_ComputeFinishedTransferSemaphore, nullptr);
	}

	if (m_GeometryFinishedSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(device, m_GeometryFinishedSemaphore, nullptr);
	}

	if (m_TransferFinishedGraphicsSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(device, m_TransferFinishedGraphicsSemaphore, nullptr);
	}

	if (m_TransferFinishedComputeSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(device, m_TransferFinishedComputeSemaphore, nullptr);
	}

	if (m_TransferStartSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(device, m_TransferStartSemaphore, nullptr);
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

	if (!createGBuffer())
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

	if (!createRayTracingRenderImages(m_pGraphicsContext->getSwapChain()->getExtent().width, m_pGraphicsContext->getSwapChain()->getExtent().height))
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

void RenderingHandlerVK::render(IScene* pScene)
{
	SceneVK* pVulkanScene	= reinterpret_cast<SceneVK*>(pScene);
	SwapChainVK* pSwapChain = m_pGraphicsContext->getSwapChain();

	pSwapChain->acquireNextImage(m_pImageAvailableSemaphores[m_CurrentFrame]);
	m_BackBufferIndex = pSwapChain->getImageIndex();

	// Prepare for frame
	m_ppGraphicsCommandBuffers[m_CurrentFrame]->reset(true);
	m_ppGraphicsCommandBuffers2[m_CurrentFrame]->reset(true);
	m_ppGraphicsCommandPools[m_CurrentFrame]->reset();
	m_ppGraphicsCommandBuffers[m_CurrentFrame]->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	m_ppComputeCommandBuffers[m_CurrentFrame]->reset(true);
	m_ppComputeCommandPools[m_CurrentFrame]->reset();
	m_ppComputeCommandBuffers[m_CurrentFrame]->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	m_ppTransferCommandBuffers[m_CurrentFrame]->reset(true);
	m_ppTransferCommandPools[m_CurrentFrame]->reset();
	m_ppTransferCommandBuffers[m_CurrentFrame]->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	const Camera& camera			= pVulkanScene->getCamera();
	LightSetup& lightsetup	= pVulkanScene->getLightSetup();
	updateBuffers(pVulkanScene, camera, lightsetup);

	DeviceVK* pDevice = m_pGraphicsContext->getDevice();
	m_ppTransferCommandBuffers[m_CurrentFrame]->end();

	//Render all the meshes
	FrameBufferVK*		pBackbuffer				= getCurrentBackBuffer();
	FrameBufferVK*		pBackbufferWithDepth	= getCurrentBackBufferWithDepth();
	CommandBufferVK*	pSecondaryCommandBuffer = m_ppCommandBuffersSecondary[m_CurrentFrame];
	CommandPoolVK*		pSecondaryCommandPool	= m_ppCommandPoolsSecondary[m_CurrentFrame];

	m_pMeshRenderer->setupFrame(m_ppGraphicsCommandBuffers[m_CurrentFrame]);
#if MULTITHREADED
	m_pMeshRenderer->beginFrame(pVulkanScene);
	m_pShadowMapRenderer->beginFrame(pVulkanScene);

	TaskDispatcher::execute([pVulkanScene, this]
		{
			auto& graphicsObjects = pVulkanScene->getGraphicsObjects();
			for (uint32_t i = 0; i < graphicsObjects.size(); i++)
			{
				const GraphicsObjectVK& graphicsObject = graphicsObjects[i];
				m_pMeshRenderer->submitMesh(graphicsObject.pMesh, graphicsObject.pMaterial, graphicsObject.MaterialParametersIndex, i);
				m_pShadowMapRenderer->submitMesh(graphicsObject.pMesh, graphicsObject.pMaterial, i);
			}
			m_pMeshRenderer->endFrame(pVulkanScene);
			m_pShadowMapRenderer->endFrame(pVulkanScene);
		});
	TaskDispatcher::execute([this]
		{
			m_pMeshRenderer->buildLightPass(m_pBackBufferRenderPass, getCurrentBackBuffer());
		});

	if (m_pImGuiRenderer)
	{
		TaskDispatcher::execute([&, this]
			{
				// Needed to begin a secondary buffer
				VkCommandBufferInheritanceInfo inheritanceInfo = {};
				inheritanceInfo.sType		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
				inheritanceInfo.pNext		= nullptr;
				inheritanceInfo.renderPass	= m_pBackBufferRenderPass->getRenderPass();
				inheritanceInfo.subpass		= 0;
				inheritanceInfo.framebuffer = pBackbuffer->getFrameBuffer();

				pSecondaryCommandBuffer->reset(false);
				pSecondaryCommandPool->reset();
				pSecondaryCommandBuffer->begin(&inheritanceInfo, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
				m_pImGuiRenderer->render(pSecondaryCommandBuffer, m_CurrentFrame);
				pSecondaryCommandBuffer->end();
			});
	}
#else
	m_pMeshRenderer->beginFrame(pVulkanScene);
	m_pShadowMapRenderer->beginFrame(pVulkanScene);

	auto& graphicsObjects = pVulkanScene->getGraphicsObjects();
	for (uint32_t i = 0; i < graphicsObjects.size(); i++)
	{
		const GraphicsObjectVK& graphicsObject = graphicsObjects[i];
		m_pMeshRenderer->submitMesh(graphicsObject.pMesh, graphicsObject.pMaterial, graphicsObject.MaterialParametersIndex, i);
		m_pShadowMapRenderer->submitMesh(graphicsObject.pMesh, graphicsObject.pMaterial, i);
	}
	m_pMeshRenderer->endFrame(pVulkanScene);
	m_pShadowMapRenderer->endFrame(pVulkanScene);

	m_pMeshRenderer->buildLightPass(m_pBackBufferRenderPass, getCurrentBackBuffer());

	// Needed to begin a secondary buffer
	VkCommandBufferInheritanceInfo inheritanceInfo = {};
	inheritanceInfo.sType		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceInfo.pNext		= nullptr;
	inheritanceInfo.renderPass	= m_pBackBufferRenderPass->getRenderPass();
	inheritanceInfo.subpass		= 0;
	inheritanceInfo.framebuffer = pBackbuffer->getFrameBuffer();

	pSecondaryCommandBuffer->reset(false);
	pSecondaryCommandPool->reset();
	pSecondaryCommandBuffer->begin(&inheritanceInfo, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);
	m_pImGuiRenderer->render(pSecondaryCommandBuffer, m_CurrentFrame);
	pSecondaryCommandBuffer->end();
#endif

	if (m_pParticleRenderer)
	{
#if MULTITHREADED
		m_pParticleRenderer->beginFrame(pVulkanScene);
		TaskDispatcher::execute([pVulkanScene, this]
			{
				submitParticles();
				m_pParticleRenderer->endFrame(pVulkanScene);
			});
#else
		m_pParticleRenderer->beginFrame(pVulkanScene);
		submitParticles();
		m_pParticleRenderer->endFrame(pVulkanScene);
#endif
	}

	{
		static bool firstFrame = true;

		const uint32_t waitCount = firstFrame ? 0 : 2;
		firstFrame = false;

		VkSemaphore				transferWaitSemphores[]		= { m_TransferStartSemaphore, m_ComputeFinishedTransferSemaphore };
		VkPipelineStageFlags	transferWaitStages[]		= { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
		VkSemaphore				transferSignalSemaphores[]	= { m_TransferFinishedGraphicsSemaphore, m_TransferFinishedComputeSemaphore };

		pDevice->executeTransfer(m_ppTransferCommandBuffers[m_CurrentFrame], transferWaitSemphores, transferWaitStages, waitCount, transferSignalSemaphores, 2);
	}

#if MULTITHREADED
	TaskDispatcher::waitForTasks();
#endif

	//Start renderpass
	VkClearValue clearValues[] = { m_ClearColor, m_ClearColor, m_ClearColor, m_ClearDepth };
	m_ppGraphicsCommandBuffers[m_CurrentFrame]->beginRenderPass(m_pGeometryRenderPass, m_pGBuffer->getFrameBuffer(), (uint32_t)m_Viewport.width, (uint32_t)m_Viewport.height, clearValues, 4, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	m_ppGraphicsCommandBuffers[m_CurrentFrame]->executeSecondary(m_pMeshRenderer->getGeometryCommandBuffer());
	m_ppGraphicsCommandBuffers[m_CurrentFrame]->endRenderPass();

	if (lightsetup.hasDirectionalLight()) {
		FrameBufferVK* pFrameBuffer = reinterpret_cast<FrameBufferVK*>(lightsetup.getDirectionalLight()->getFrameBuffer());
		const VkViewport& viewport = m_pShadowMapRenderer->getViewport();

		m_ppGraphicsCommandBuffers[m_CurrentFrame]->beginRenderPass(m_pShadowMapRenderPass, pFrameBuffer, (uint32_t)viewport.width, (uint32_t)viewport.height, &m_ClearDepth, 1, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->executeSecondary(m_pShadowMapRenderer->getCommandBuffer(m_CurrentFrame));
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->endRenderPass();
	}

	if (m_pVolumetricLightRenderer) {
		m_pVolumetricLightRenderer->beginFrame(pScene);

		m_pVolumetricLightRenderer->updateBuffers();
		m_pVolumetricLightRenderer->renderLightBuffer();
		m_pVolumetricLightRenderer->applyLightBuffer(m_pBackBufferRenderPass, pBackbuffer);

		m_pVolumetricLightRenderer->endFrame(pScene);
	}

	if (m_pRayTracer)
	{
		uint32_t computeQueueIndex	= pDevice->getQueueFamilyIndices().computeFamily.value();
		uint32_t graphicsQueueIndex = pDevice->getQueueFamilyIndices().graphicsFamily.value();

		constexpr uint32_t IMAGE_BARRIER_COUNT = 6;
		VkImageMemoryBarrier imageBarriers[IMAGE_BARRIER_COUNT] =
		{
			createVkImageMemoryBarrier(m_pGBuffer->getDepthImage()->getImage(), VK_ACCESS_MEMORY_READ_BIT, 0, graphicsQueueIndex, computeQueueIndex,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1, 1),
			createVkImageMemoryBarrier(m_pGBuffer->getColorImage(0)->getImage(), VK_ACCESS_MEMORY_READ_BIT, 0, graphicsQueueIndex, computeQueueIndex,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
			createVkImageMemoryBarrier(m_pGBuffer->getColorImage(1)->getImage(), VK_ACCESS_MEMORY_READ_BIT, 0, graphicsQueueIndex, computeQueueIndex,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
			createVkImageMemoryBarrier(m_pGBuffer->getColorImage(2)->getImage(), VK_ACCESS_MEMORY_READ_BIT, 0, graphicsQueueIndex, computeQueueIndex,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
			createVkImageMemoryBarrier(m_pRadianceImage->getImage(), VK_ACCESS_MEMORY_READ_BIT, 0, graphicsQueueIndex, computeQueueIndex,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
			createVkImageMemoryBarrier(m_pGlossyImage->getImage(), VK_ACCESS_MEMORY_READ_BIT, 0, graphicsQueueIndex, computeQueueIndex,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,  VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
		};

		m_ppGraphicsCommandBuffers[m_CurrentFrame]->imageMemoryBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, IMAGE_BARRIER_COUNT, imageBarriers);
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->end();

		{
			VkSemaphore geometryWaitSemphores[] = { m_TransferFinishedGraphicsSemaphore };
			VkPipelineStageFlags geometryWaitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };

			VkSemaphore signalSemaphores[] = { m_GeometryFinishedSemaphore, m_TransferStartSemaphore };
			pDevice->executeGraphics(m_ppGraphicsCommandBuffers[m_CurrentFrame], geometryWaitSemphores, geometryWaitStages, 1, signalSemaphores, 2);
		}

		//Prepare seconds graphics commandbuffer
		m_ppGraphicsCommandBuffers2[m_CurrentFrame]->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		{
			VkImageMemoryBarrier imageBarriers[IMAGE_BARRIER_COUNT] =
			{
				createVkImageMemoryBarrier(m_pGBuffer->getDepthImage()->getImage(), 0, VK_ACCESS_MEMORY_READ_BIT, graphicsQueueIndex, computeQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pGBuffer->getColorImage(0)->getImage(), 0, VK_ACCESS_MEMORY_READ_BIT, graphicsQueueIndex, computeQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pGBuffer->getColorImage(1)->getImage(), 0, VK_ACCESS_MEMORY_READ_BIT, graphicsQueueIndex, computeQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pGBuffer->getColorImage(2)->getImage(), 0, VK_ACCESS_MEMORY_READ_BIT, graphicsQueueIndex, computeQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pRadianceImage->getImage(), 0, VK_ACCESS_MEMORY_WRITE_BIT, graphicsQueueIndex, computeQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pGlossyImage->getImage(), 0, VK_ACCESS_MEMORY_WRITE_BIT, graphicsQueueIndex, computeQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,  VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
			};
			m_ppComputeCommandBuffers[m_CurrentFrame]->imageMemoryBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, IMAGE_BARRIER_COUNT, imageBarriers);
		}

		m_pRayTracer->render(pScene);
		m_ppComputeCommandBuffers[m_CurrentFrame]->executeSecondary(m_pRayTracer->getComputeCommandBuffer());

		{
			VkImageMemoryBarrier imageBarriers[IMAGE_BARRIER_COUNT] =
			{
				createVkImageMemoryBarrier(m_pGBuffer->getDepthImage()->getImage(), VK_ACCESS_MEMORY_READ_BIT, 0, computeQueueIndex, graphicsQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pGBuffer->getColorImage(0)->getImage(), VK_ACCESS_MEMORY_READ_BIT, 0, computeQueueIndex, graphicsQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pGBuffer->getColorImage(1)->getImage(), VK_ACCESS_MEMORY_READ_BIT, 0, computeQueueIndex, graphicsQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pGBuffer->getColorImage(2)->getImage(), VK_ACCESS_MEMORY_READ_BIT, 0, computeQueueIndex, graphicsQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pRadianceImage->getImage(), VK_ACCESS_MEMORY_WRITE_BIT, 0, computeQueueIndex, graphicsQueueIndex,
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pGlossyImage->getImage(), VK_ACCESS_MEMORY_WRITE_BIT, 0, computeQueueIndex, graphicsQueueIndex,
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
			};
			m_ppComputeCommandBuffers[m_CurrentFrame]->imageMemoryBarrier(VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, IMAGE_BARRIER_COUNT, imageBarriers);
		}

		{
			VkImageMemoryBarrier imageBarriers[IMAGE_BARRIER_COUNT] =
			{
				createVkImageMemoryBarrier(m_pGBuffer->getDepthImage()->getImage(), 0, VK_ACCESS_MEMORY_READ_BIT, computeQueueIndex, graphicsQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pGBuffer->getColorImage(0)->getImage(), 0, VK_ACCESS_MEMORY_READ_BIT, computeQueueIndex, graphicsQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pGBuffer->getColorImage(1)->getImage(), 0, VK_ACCESS_MEMORY_READ_BIT, computeQueueIndex, graphicsQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pGBuffer->getColorImage(2)->getImage(), 0, VK_ACCESS_MEMORY_READ_BIT, computeQueueIndex, graphicsQueueIndex,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pRadianceImage->getImage(), 0, VK_ACCESS_MEMORY_READ_BIT, computeQueueIndex, graphicsQueueIndex,
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
				createVkImageMemoryBarrier(m_pGlossyImage->getImage(), 0, VK_ACCESS_MEMORY_READ_BIT, computeQueueIndex, graphicsQueueIndex,
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1, 1),
			};
			m_ppGraphicsCommandBuffers2[m_CurrentFrame]->imageMemoryBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, IMAGE_BARRIER_COUNT, imageBarriers);
		}
	}
	else
	{
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->end();
		{
			VkSemaphore geometryWaitSemphores[] = { m_TransferFinishedGraphicsSemaphore };
			VkPipelineStageFlags geometryWaitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };

			VkSemaphore signalSemaphores[] = { m_GeometryFinishedSemaphore, m_TransferStartSemaphore };
			pDevice->executeGraphics(m_ppGraphicsCommandBuffers[m_CurrentFrame], geometryWaitSemphores, geometryWaitStages, 1, signalSemaphores, 2);
		}
		m_ppGraphicsCommandBuffers2[m_CurrentFrame]->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	}

	if (m_pVolumetricLightRenderer) {
		RenderPassVK* pLightBufferPass = m_pVolumetricLightRenderer->getLightBufferPass();
		FrameBufferVK* pLightFrameBuffer = m_pVolumetricLightRenderer->getLightFrameBuffer();
		const VkViewport& viewport = m_pVolumetricLightRenderer->getViewport();
		VkClearValue clearValue = m_pVolumetricLightRenderer->getLightBufferClearColor();

		m_ppGraphicsCommandBuffers2[m_CurrentFrame]->beginRenderPass(pLightBufferPass, pLightFrameBuffer, (uint32_t)viewport.width, (uint32_t)viewport.height, &clearValue, 1, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		m_ppGraphicsCommandBuffers2[m_CurrentFrame]->executeSecondary(m_pVolumetricLightRenderer->getCommandBufferBuildPass(m_CurrentFrame));
		m_ppGraphicsCommandBuffers2[m_CurrentFrame]->endRenderPass();
	}

	//TODO: Combine these into one renderpass

	//Gather all renderer's data and finalize the frame
	m_ppGraphicsCommandBuffers2[m_CurrentFrame]->beginRenderPass(m_pBackBufferRenderPass, pBackbuffer, (uint32_t)m_Viewport.width, (uint32_t)m_Viewport.height, nullptr, 0, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	m_ppGraphicsCommandBuffers2[m_CurrentFrame]->executeSecondary(m_pMeshRenderer->getLightCommandBuffer());

	if (m_pVolumetricLightRenderer) {
		m_ppGraphicsCommandBuffers2[m_CurrentFrame]->executeSecondary(m_pVolumetricLightRenderer->getCommandBufferApplyPass(m_CurrentFrame));
	}

	m_ppGraphicsCommandBuffers2[m_CurrentFrame]->endRenderPass();

	//Render particles
	if (m_pParticleRenderer)
	{
		m_ppGraphicsCommandBuffers2[m_CurrentFrame]->beginRenderPass(m_pParticleRenderPass, pBackbufferWithDepth, (uint32_t)m_Viewport.width, (uint32_t)m_Viewport.height, nullptr, 0, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		m_ppGraphicsCommandBuffers2[m_CurrentFrame]->executeSecondary(m_pParticleRenderer->getCommandBuffer(m_CurrentFrame));
		m_ppGraphicsCommandBuffers2[m_CurrentFrame]->endRenderPass();
	}

	//Render UI
	m_ppGraphicsCommandBuffers2[m_CurrentFrame]->beginRenderPass(m_pUIRenderPass, pBackbuffer, (uint32_t)m_Viewport.width, (uint32_t)m_Viewport.height, nullptr, 0, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
	m_ppGraphicsCommandBuffers2[m_CurrentFrame]->executeSecondary(pSecondaryCommandBuffer);
	m_ppGraphicsCommandBuffers2[m_CurrentFrame]->endRenderPass();

	m_ppGraphicsCommandBuffers2[m_CurrentFrame]->end();
	m_ppComputeCommandBuffers[m_CurrentFrame]->end();

	// Execute commandbuffer
	{
		VkSemaphore graphicsSignalSemaphores[]		= { m_pRenderFinishedSemaphores[m_CurrentFrame] };
		VkSemaphore graphicsWaitSemaphores[]		= { m_pImageAvailableSemaphores[m_CurrentFrame], m_ComputeFinishedGraphicsSemaphore };
		VkPipelineStageFlags graphicswaitStages[]	= { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };

		VkSemaphore computeSignalSemaphores[]		= { m_ComputeFinishedGraphicsSemaphore, m_ComputeFinishedTransferSemaphore };
		VkSemaphore computeWaitSemaphores[]			= { m_GeometryFinishedSemaphore, m_TransferFinishedComputeSemaphore };
		VkPipelineStageFlags computeWaitStages[]	= { VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV };

		pDevice->executeCompute(m_ppComputeCommandBuffers[m_CurrentFrame], computeWaitSemaphores, computeWaitStages, 2, computeSignalSemaphores, 2);
		pDevice->executeGraphics(m_ppGraphicsCommandBuffers2[m_CurrentFrame], graphicsWaitSemaphores, graphicswaitStages, 2, graphicsSignalSemaphores, 1);
	}

	swapBuffers();
}

void RenderingHandlerVK::onWindowResize(uint32_t width, uint32_t height)
{
	m_pGraphicsContext->getDevice()->wait();
	releaseBackBuffers();

	m_pGraphicsContext->getSwapChain()->resize(width, height);

	m_pGBuffer->resize(width, height);

	createRayTracingRenderImages(width, height);

	if (m_pMeshRenderer)
	{
		m_pMeshRenderer->onWindowResize(width, height);
		m_pMeshRenderer->setRayTracingResultImages(m_pRadianceImageView, m_pGlossyImageView);
	}

	if (m_pRayTracer)
	{
		m_pRayTracer->onWindowResize(width / m_RayTracingResolutionDenominator, height / m_RayTracingResolutionDenominator);
		m_pRayTracer->setGBufferTextures(m_pGBuffer);
		m_pRayTracer->setRayTracingResultTextures(m_pRadianceImage, m_pRadianceImageView, m_pGlossyImage, m_pGlossyImageView, m_pGraphicsContext->getSwapChain()->getExtent().width, m_pGraphicsContext->getSwapChain()->getExtent().height);
	}

	createBackBuffers();

	if (m_pParticleEmitterHandler) {
		m_pParticleEmitterHandler->onWindowResize();
	}

	if (m_pShadowMapRenderer) {
		m_pShadowMapRenderer->onWindowResize(width, height);
	}

	if (m_pVolumetricLightRenderer) {
		m_pVolumetricLightRenderer->onWindowResize(width, height);
	}
}

void RenderingHandlerVK::onSceneUpdated(IScene* pScene)
{
	SceneVK* pSceneVK = reinterpret_cast<SceneVK*>(pScene);
	bool update = pSceneVK->updateSceneData();

	if (m_pRayTracer)
	{
		if (update)
		{
			m_pRayTracer->setSceneData(pScene);
		}
	}
}

void RenderingHandlerVK::setRayTracingResolutionDenominator(uint32_t denom)
{
	m_RayTracingResolutionDenominator = denom;
	m_pGraphicsContext->getDevice()->wait();

	VkExtent2D extent = m_pGBuffer->getExtent();

	createRayTracingRenderImages(extent.width, extent.height);

	if (m_pMeshRenderer)
	{
		m_pMeshRenderer->setRayTracingResultImages(m_pRadianceImageView, m_pGlossyImageView);
	}

	if (m_pRayTracer)
	{
		m_pRayTracer->onWindowResize(extent.width / m_RayTracingResolutionDenominator, extent.height / m_RayTracingResolutionDenominator);
		m_pRayTracer->setRayTracingResultTextures(m_pRadianceImage, m_pRadianceImageView, m_pGlossyImage, m_pGlossyImageView, m_pGraphicsContext->getSwapChain()->getExtent().width, m_pGraphicsContext->getSwapChain()->getExtent().height);
	}
}

void RenderingHandlerVK::swapBuffers()
{
    m_pGraphicsContext->swapBuffers(m_pRenderFinishedSemaphores[m_CurrentFrame]);
	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RenderingHandlerVK::drawProfilerUI()
{
	if (m_pMeshRenderer)
	{
		m_pMeshRenderer->getGeometryProfiler()->drawResults();
		m_pMeshRenderer->getLightProfiler()->drawResults();
	}

	if (m_pShadowMapRenderer) {
		m_pShadowMapRenderer->getProfiler()->drawResults();
	}

	if (m_pParticleRenderer)
	{
		m_pParticleRenderer->getProfiler()->drawResults();
	}

	if (m_pRayTracer)
	{
		m_pRayTracer->getProfiler()->drawResults();
	}

	if (m_pVolumetricLightRenderer) {
		m_pVolumetricLightRenderer->getProfiler()->drawResults();
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

	if (m_pShadowMapRenderer) {
		m_pShadowMapRenderer->setViewport(width, height, minDepth, maxDepth, topX, topY);
	}

	if (m_pVolumetricLightRenderer) {
		m_pVolumetricLightRenderer->setViewport(width, height, minDepth, maxDepth, topX, topY);
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
		m_pRayTracer->setBRDFLookUp(m_pMeshRenderer->getBRDFLookUp());
	}
}

bool RenderingHandlerVK::createBackBuffers()
{
    SwapChainVK* pSwapChain = m_pGraphicsContext->getSwapChain();
	DeviceVK* pDevice		= m_pGraphicsContext->getDevice();

	VkExtent2D extent = pSwapChain->getExtent();
	ImageViewVK* pDepthStencilView = m_pGBuffer->getDepthAttachment();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppBackbuffers[i] = DBG_NEW FrameBufferVK(pDevice);
		m_ppBackbuffers[i]->addColorAttachment(pSwapChain->getImageView(i));
		if (!m_ppBackbuffers[i]->finalize(m_pBackBufferRenderPass, extent.width, extent.height))
		{
			return false;
		}

		m_ppBackBuffersWithDepth[i] = DBG_NEW FrameBufferVK(pDevice);
		m_ppBackBuffersWithDepth[i]->addColorAttachment(pSwapChain->getImageView(i));
		m_ppBackBuffersWithDepth[i]->setDepthStencilAttachment(pDepthStencilView);
		if (!m_ppBackBuffersWithDepth[i]->finalize(m_pParticleRenderPass, extent.width, extent.height))
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
	const uint32_t computeQueueFamilyIndex	= pDevice->getQueueFamilyIndices().computeFamily.value();
	const uint32_t transferQueueFamilyIndex = pDevice->getQueueFamilyIndices().transferFamily.value();

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
        m_ppGraphicsCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, graphicsQueueFamilyIndex);
		if (!m_ppGraphicsCommandPools[i]->init())
		{
			return false;
		}
		std::string name = "GraphicsCommandPool[" + std::to_string(i) + "]";
		m_ppGraphicsCommandPools[i]->setName(name.c_str());

        m_ppGraphicsCommandBuffers[i] = m_ppGraphicsCommandPools[i]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        if (m_ppGraphicsCommandBuffers[i] == nullptr)
		{
            return false;
        }
		name = "GraphicsCommandBuffer[" + std::to_string(i) + "]";
		m_ppGraphicsCommandBuffers[i]->setName(name.c_str());

		m_ppGraphicsCommandBuffers2[i] = m_ppGraphicsCommandPools[i]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		if (m_ppGraphicsCommandBuffers2[i] == nullptr)
		{
			return false;
		}
		name = "GraphicsCommandBuffer2[" + std::to_string(i) + "]";
		m_ppGraphicsCommandBuffers2[i]->setName(name.c_str());

		//Compute
		m_ppComputeCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, computeQueueFamilyIndex);
		if (!m_ppComputeCommandPools[i]->init())
		{
			return false;
		}
		name = "ComputeCommandPool[" + std::to_string(i) + "]";
		m_ppComputeCommandPools[i]->setName(name.c_str());

		m_ppComputeCommandBuffers[i] = m_ppComputeCommandPools[i]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		if (m_ppComputeCommandBuffers[i] == nullptr)
		{
			return false;
		}

		name = "ComputeCommandBuffer[" + std::to_string(i) + "]";
		m_ppComputeCommandBuffers[i]->setName(name.c_str());

		//Transfer
		m_ppTransferCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, transferQueueFamilyIndex);
		if (!m_ppTransferCommandPools[i]->init())
		{
			return false;
		}
		name = "TransferCommandPool[" + std::to_string(i) + "]";
		m_ppTransferCommandPools[i]->setName(name.c_str());

		m_ppTransferCommandBuffers[i] = m_ppTransferCommandPools[i]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		if (m_ppTransferCommandBuffers[i] == nullptr)
		{
			return false;
		}
		name = "TransferCommandBuffer[" + std::to_string(i) + "]";
		m_ppTransferCommandBuffers[i]->setName(name.c_str());

		//Secondary
        m_ppCommandPoolsSecondary[i] = DBG_NEW CommandPoolVK(pDevice, graphicsQueueFamilyIndex);
		if (!m_ppCommandPoolsSecondary[i]->init())
		{
			return false;
		}
		name = "SecondaryCommandPool[" + std::to_string(i) + "]";
		m_ppCommandPoolsSecondary[i]->setName(name.c_str());

        m_ppCommandBuffersSecondary[i] = m_ppCommandPoolsSecondary[i]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY);
        if (m_ppCommandBuffersSecondary[i] == nullptr)
		{
            return false;
        }
		name = "SecondaryCommandBuffer[" + std::to_string(i) + "]";
		m_ppCommandBuffersSecondary[i]->setName(name.c_str());
    }

	return true;
}

bool RenderingHandlerVK::createRenderPasses()
{
	//Create Backbuffer Renderpass
	m_pUIRenderPass			= DBG_NEW RenderPassVK(m_pGraphicsContext->getDevice());
	m_pBackBufferRenderPass = DBG_NEW RenderPassVK(m_pGraphicsContext->getDevice());
	VkAttachmentDescription description = {};
	description.format			= VK_FORMAT_B8G8R8A8_UNORM;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	m_pBackBufferRenderPass->addAttachment(description);

	description.loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD;
	description.initialLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	description.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	m_pUIRenderPass->addAttachment(description);

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment	= 0;
	colorAttachmentRef.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	m_pBackBufferRenderPass->addSubpass(&colorAttachmentRef, 1, nullptr);
	m_pUIRenderPass->addSubpass(&colorAttachmentRef, 1, nullptr);

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

	m_pUIRenderPass->addSubpassDependency(dependency);
	if (!m_pUIRenderPass->finalize())
	{
		return false;
	}

	//Create Backbuffer Renderpass
	m_pParticleRenderPass = DBG_NEW RenderPassVK(m_pGraphicsContext->getDevice());
	description.format			= VK_FORMAT_B8G8R8A8_UNORM;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD;
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	description.finalLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	m_pParticleRenderPass->addAttachment(description);

	description.format			= VK_FORMAT_D32_SFLOAT;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD;
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	description.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pParticleRenderPass->addAttachment(description);

	colorAttachmentRef.attachment	= 0;
	colorAttachmentRef.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentRef = {};
	depthStencilAttachmentRef.attachment	= 1;
	depthStencilAttachmentRef.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pParticleRenderPass->addSubpass(&colorAttachmentRef, 1, &depthStencilAttachmentRef);

	dependency.srcSubpass		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass		= 0;
	dependency.dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;
	dependency.srcAccessMask	= 0;
	dependency.dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	m_pParticleRenderPass->addSubpassDependency(dependency);
	if (!m_pParticleRenderPass->finalize())
	{
		return false;
	}

	//Create Geometry Renderpass
	m_pGeometryRenderPass = DBG_NEW RenderPassVK(m_pGraphicsContext->getDevice());

	//Albedo + AO
	description.format			= VK_FORMAT_R8G8B8A8_UNORM;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_pGeometryRenderPass->addAttachment(description);

	//Normals + Metallic + Roughness
	description.format			= VK_FORMAT_R16G16B16A16_SFLOAT;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_pGeometryRenderPass->addAttachment(description);

	//Motion
	description.format			= VK_FORMAT_R16G16B16A16_SFLOAT;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_pGeometryRenderPass->addAttachment(description);

	//Depth
	description.format			= VK_FORMAT_D32_SFLOAT;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_pGeometryRenderPass->addAttachment(description);

	constexpr uint32_t COLOR_REF_COUNT = 3;
	VkAttachmentReference colorAttachmentRefs[COLOR_REF_COUNT];
	//Albedo + AO
	colorAttachmentRefs[0].attachment	= 0;
	colorAttachmentRefs[0].layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//Normals + Metallic + Roughness
	colorAttachmentRefs[1].attachment	= 1;
	colorAttachmentRefs[1].layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//Velocity
	colorAttachmentRefs[2].attachment	= 2;
	colorAttachmentRefs[2].layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	depthStencilAttachmentRef.attachment	= COLOR_REF_COUNT;
	depthStencilAttachmentRef.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pGeometryRenderPass->addSubpass(colorAttachmentRefs, COLOR_REF_COUNT, &depthStencilAttachmentRef);

	dependency.dependencyFlags	= VK_DEPENDENCY_BY_REGION_BIT;
	dependency.srcSubpass		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass		= 0;
	dependency.srcStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
	dependency.dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	m_pGeometryRenderPass->addSubpassDependency(dependency);

	dependency.srcSubpass		= 0;
	dependency.dstSubpass		= VK_SUBPASS_EXTERNAL;
	dependency.srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.srcAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask	= VK_ACCESS_MEMORY_READ_BIT;
	m_pGeometryRenderPass->addSubpassDependency(dependency);

	if (!m_pGeometryRenderPass->finalize()) {
		return false;
	}

	// Shadow map pass
	m_pShadowMapRenderPass = DBG_NEW RenderPassVK(m_pGraphicsContext->getDevice());

	// Depth, shadow map
	description.format			= VK_FORMAT_D32_SFLOAT;
	description.samples			= VK_SAMPLE_COUNT_1_BIT;
	description.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	description.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	description.finalLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_pShadowMapRenderPass->addAttachment(description);

	depthStencilAttachmentRef.attachment	= 0;
	depthStencilAttachmentRef.layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pShadowMapRenderPass->addSubpass(nullptr, 0, &depthStencilAttachmentRef);

	return m_pShadowMapRenderPass->finalize();
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
		VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_pImageAvailableSemaphores[i]), "Failed to create semaphores for Frame");
		VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_pRenderFinishedSemaphores[i]), "Failed to create semaphores for Frame");
	}

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ComputeFinishedGraphicsSemaphore), "Failed to create semaphores for Compute Finsihed Graphics");
	VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ComputeFinishedTransferSemaphore), "Failed to create semaphores for Compute Finsihed Transfer");
	VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_GeometryFinishedSemaphore), "Failed to create semaphores for Geometry Pass");
	VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_TransferStartSemaphore), "Failed to create semaphores for Start Transfer");
	VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_TransferFinishedGraphicsSemaphore), "Failed to create semaphores for Finished Transfer Graphics");
	VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_TransferFinishedComputeSemaphore), "Failed to create semaphores for Finished Transfer Compute");

	return true;
}

void RenderingHandlerVK::releaseBackBuffers()
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		SAFEDELETE(m_ppBackbuffers[i]);
		SAFEDELETE(m_ppBackBuffersWithDepth[i]);
	}
}

void RenderingHandlerVK::updateBuffers(SceneVK* pScene, const Camera& camera, const LightSetup& lightSetup)
{
	DeviceVK* pDevice = m_pGraphicsContext->getDevice();
	const uint32_t graphicsQueueFamilyIndex = pDevice->getQueueFamilyIndices().graphicsFamily.value();
	const uint32_t computeQueueFamilyIndex	= pDevice->getQueueFamilyIndices().computeFamily.value();
	const uint32_t transferQueueFamilyIndex = pDevice->getQueueFamilyIndices().transferFamily.value();

	//Transfer ownership to transfer-queue
	constexpr uint32_t barrierCount = 4;
	{
		VkBufferMemoryBarrier barriers[barrierCount] =
		{
			createVkBufferMemoryBarrier(m_pCameraBufferGraphics->getBuffer(),
				VK_ACCESS_MEMORY_READ_BIT, 0, graphicsQueueFamilyIndex, transferQueueFamilyIndex, 0, VK_WHOLE_SIZE),
			createVkBufferMemoryBarrier(m_pLightBufferGraphics->getBuffer(),
				VK_ACCESS_MEMORY_READ_BIT, 0, graphicsQueueFamilyIndex, transferQueueFamilyIndex, 0, VK_WHOLE_SIZE),

			createVkBufferMemoryBarrier(m_pCameraBufferCompute->getBuffer(),
				VK_ACCESS_MEMORY_READ_BIT, 0, computeQueueFamilyIndex, transferQueueFamilyIndex, 0, VK_WHOLE_SIZE),
			createVkBufferMemoryBarrier(m_pLightBufferCompute->getBuffer(),
				VK_ACCESS_MEMORY_READ_BIT, 0, computeQueueFamilyIndex, transferQueueFamilyIndex, 0, VK_WHOLE_SIZE)
		};

		m_ppGraphicsCommandBuffers[m_CurrentFrame]->bufferMemoryBarrier(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 2, barriers);
		m_ppComputeCommandBuffers[m_CurrentFrame]->bufferMemoryBarrier(VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 2, &barriers[2]);

		for (uint32_t i = 0; i < barrierCount; i++)
		{
			barriers[i].srcAccessMask = 0;
			barriers[i].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		m_ppTransferCommandBuffers[m_CurrentFrame]->bufferMemoryBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, barrierCount, barriers);
	}

	// Update camera buffers
	m_CameraBuffer.LastProjection	= m_CameraBuffer.Projection;
	m_CameraBuffer.LastView			= m_CameraBuffer.View;
	m_CameraBuffer.Projection		= camera.getProjectionMat();
	m_CameraBuffer.View				= camera.getViewMat();
	m_CameraBuffer.InvView			= camera.getViewInvMat();
	m_CameraBuffer.InvProjection	= camera.getProjectionInvMat();
	m_CameraBuffer.Position			= glm::vec4(camera.getPosition(), 1.0f);
	m_CameraBuffer.Right			= glm::vec4(camera.getRightVec(), 0.0f);
	m_CameraBuffer.Up				= glm::vec4(camera.getUpVec(), 0.0f);
	m_ppTransferCommandBuffers[m_CurrentFrame]->updateBuffer(m_pCameraBufferGraphics, 0, (const void*)&m_CameraBuffer, sizeof(CameraBuffer));
	m_ppTransferCommandBuffers[m_CurrentFrame]->updateBuffer(m_pCameraBufferCompute, 0, (const void*)&m_CameraBuffer, sizeof(CameraBuffer));

	const uint32_t lightBufferSize = sizeof(PointLight) * lightSetup.getPointLightCount();
	m_ppTransferCommandBuffers[m_CurrentFrame]->updateBuffer(m_pLightBufferGraphics, 0, (const void*)lightSetup.getPointLights(), lightBufferSize);
	m_ppTransferCommandBuffers[m_CurrentFrame]->updateBuffer(m_pLightBufferCompute, 0, (const void*)lightSetup.getPointLights(), lightBufferSize);

	pScene->copySceneData(m_ppTransferCommandBuffers[m_CurrentFrame]);

	//Transfer back from transfer queue
	{
		VkBufferMemoryBarrier barriers[barrierCount] =
		{
			createVkBufferMemoryBarrier(m_pCameraBufferGraphics->getBuffer(),
				VK_ACCESS_TRANSFER_WRITE_BIT, 0, transferQueueFamilyIndex, graphicsQueueFamilyIndex, 0, VK_WHOLE_SIZE),
			createVkBufferMemoryBarrier(m_pLightBufferGraphics->getBuffer(),
				VK_ACCESS_TRANSFER_WRITE_BIT, 0, transferQueueFamilyIndex, graphicsQueueFamilyIndex, 0, VK_WHOLE_SIZE),

			createVkBufferMemoryBarrier(m_pCameraBufferCompute->getBuffer(),
				VK_ACCESS_TRANSFER_WRITE_BIT, 0, transferQueueFamilyIndex, computeQueueFamilyIndex, 0, VK_WHOLE_SIZE),
			createVkBufferMemoryBarrier(m_pLightBufferCompute->getBuffer(),
				VK_ACCESS_TRANSFER_WRITE_BIT, 0, transferQueueFamilyIndex, computeQueueFamilyIndex, 0, VK_WHOLE_SIZE)
		};
		m_ppTransferCommandBuffers[m_CurrentFrame]->bufferMemoryBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, barrierCount, barriers);

		for (uint32_t i = 0; i < barrierCount; i++)
		{
			barriers[i].srcAccessMask = 0;
			barriers[i].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		}
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->bufferMemoryBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 2, barriers);
		m_ppComputeCommandBuffers[m_CurrentFrame]->bufferMemoryBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, 2, &barriers[2]);
	}

	// Update particle buffers
	if (m_pParticleEmitterHandler) {
		m_pParticleEmitterHandler->updateRenderingBuffers(this);
	}

	if (m_pShadowMapRenderer) {
		m_pShadowMapRenderer->updateBuffers(reinterpret_cast<SceneVK*>(m_pScene));
	}
}

void RenderingHandlerVK::submitParticles()
{
	ParticleEmitterHandlerVK* pEmitterHandler = reinterpret_cast<ParticleEmitterHandlerVK*>(m_pParticleEmitterHandler);

	// Transfer depth buffer and particle buffer ownerships between the compute and graphics queue
	if (pEmitterHandler->gpuComputed()) {
		m_ppGraphicsCommandBuffers[m_CurrentFrame]->releaseImageOwnership(
			m_pGBuffer->getDepthImage(),
			VK_ACCESS_MEMORY_READ_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT);

		for (ParticleEmitter* pEmitter : pEmitterHandler->getParticleEmitters()) {
			// Release buffers from graphics after the geometry pass. This will allow the compute queue to update the particles
			pEmitterHandler->releaseFromGraphics(reinterpret_cast<BufferVK*>(pEmitter->getPositionsBuffer()), m_ppGraphicsCommandBuffers[m_CurrentFrame]);

			// Particle buffers will be acquired once the particles have been updated
			pEmitterHandler->acquireForGraphics(reinterpret_cast<BufferVK*>(pEmitter->getPositionsBuffer()), m_ppGraphicsCommandBuffers[m_CurrentFrame]);
		}

		m_ppGraphicsCommandBuffers[m_CurrentFrame]->acquireImageOwnership(
			m_pGBuffer->getDepthImage(),
			VK_ACCESS_MEMORY_READ_BIT,
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().computeFamily.value(),
			m_pGraphicsContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value(),
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT);
	}

	for (ParticleEmitter* pEmitter : pEmitterHandler->getParticleEmitters()) {
		m_pParticleRenderer->submitParticles(pEmitter);
	}
}

bool RenderingHandlerVK::createBuffers()
{
	// Create CameraBuffers
	BufferParams cameraBufferParams = {};
	cameraBufferParams.Usage			= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	cameraBufferParams.SizeInBytes		= sizeof(CameraBuffer);
	cameraBufferParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	cameraBufferParams.IsExclusive		= true;

	m_pCameraBufferGraphics = DBG_NEW BufferVK(m_pGraphicsContext->getDevice());
	if (!m_pCameraBufferGraphics->init(cameraBufferParams))
	{
		LOG("Failed to create CameraBuffer for Graphics");
		return false;
	}
	else
	{
		m_pCameraBufferGraphics->setName("CameraBuffer Graphics");
	}

	m_pCameraBufferCompute = DBG_NEW BufferVK(m_pGraphicsContext->getDevice());
	if (!m_pCameraBufferCompute->init(cameraBufferParams))
	{
		LOG("Failed to create CameraBuffer for Compute");
		return false;
	}
	else
	{
		m_pCameraBufferCompute->setName("CameraBuffer Compute");
	}

	// Create LightBuffers
	BufferParams lightBufferParams = {};
	lightBufferParams.Usage				= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	lightBufferParams.SizeInBytes		= sizeof(PointLight) * MAX_POINTLIGHTS;
	lightBufferParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	lightBufferParams.IsExclusive		= true;

	m_pLightBufferGraphics = DBG_NEW BufferVK(m_pGraphicsContext->getDevice());
	if (!m_pLightBufferGraphics->init(lightBufferParams))
	{
		LOG("Failed to create LightBuffer for Graphics");
		return false;
	}
	else
	{
		m_pLightBufferGraphics->setName("LightBuffer Graphics");
	}

	m_pLightBufferCompute = DBG_NEW BufferVK(m_pGraphicsContext->getDevice());
	if (!m_pLightBufferCompute->init(lightBufferParams))
	{
		LOG("Failed to create LightBuffer for Compute");
		return false;
	}
	else
	{
		m_pLightBufferCompute->setName("LightBuffer Compute");
	}

	return true;
}

bool RenderingHandlerVK::createRayTracingRenderImages(uint32_t width, uint32_t height)
{
	SAFEDELETE(m_pRadianceImage);
	SAFEDELETE(m_pRadianceImageView);

	SAFEDELETE(m_pGlossyImage);
	SAFEDELETE(m_pGlossyImageView);

	ImageParams imageParams = {};
	imageParams.Type = VK_IMAGE_TYPE_2D;
	imageParams.Format = VK_FORMAT_R16G16B16A16_SFLOAT; //Todo: What format should this be?
	imageParams.Extent.width = width / m_RayTracingResolutionDenominator;
	imageParams.Extent.height = height / m_RayTracingResolutionDenominator;
	imageParams.Extent.depth = 1;
	imageParams.MipLevels = 1;
	imageParams.ArrayLayers = 1;
	imageParams.Samples = VK_SAMPLE_COUNT_1_BIT;
	imageParams.Usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	m_pRadianceImage = DBG_NEW ImageVK(m_pGraphicsContext->getDevice());
	m_pRadianceImage->init(imageParams);

	m_pGlossyImage = DBG_NEW ImageVK(m_pGraphicsContext->getDevice());
	m_pGlossyImage->init(imageParams);

	ImageViewParams imageViewParams = {};
	imageViewParams.Type = VK_IMAGE_VIEW_TYPE_2D;
	imageViewParams.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewParams.FirstMipLevel = 0;
	imageViewParams.MipLevels = 1;
	imageViewParams.FirstLayer = 0;
	imageViewParams.LayerCount = 1;

	m_pRadianceImageView = DBG_NEW ImageViewVK(m_pGraphicsContext->getDevice(), m_pRadianceImage);
	m_pRadianceImageView->init(imageViewParams);

	m_pGlossyImageView = DBG_NEW ImageViewVK(m_pGraphicsContext->getDevice(), m_pGlossyImage);
	m_pGlossyImageView->init(imageViewParams);

	CommandBufferVK* pTempCommandBuffer = m_ppComputeCommandPools[0]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	pTempCommandBuffer->reset(true);
	pTempCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	pTempCommandBuffer->transitionImageLayout(m_pRadianceImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);
	pTempCommandBuffer->transitionImageLayout(m_pGlossyImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1);
	pTempCommandBuffer->end();

	m_pGraphicsContext->getDevice()->executeCompute(pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
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
	m_pGBuffer->setDepthAttachmentFormat(VK_FORMAT_D32_SFLOAT);
	return m_pGBuffer->finalize(m_pGeometryRenderPass, extent.width, extent.height);
}

