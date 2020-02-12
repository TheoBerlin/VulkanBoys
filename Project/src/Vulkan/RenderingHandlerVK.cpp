#include "RenderingHandlerVK.hpp"

#include "Common/IGraphicsContext.h"
#include "Common/IRenderer.h"
#include "Common/IImgui.h"
#include "Vulkan/CommandBufferVK.h"
#include "Vulkan/CommandPoolVK.h"
#include "Vulkan/FrameBufferVK.h"
#include "Vulkan/GraphicsContextVK.h"
#include "Vulkan/PipelineVK.h"
#include "Vulkan/RenderPassVK.h"
#include "Vulkan/SwapChainVK.h"

RenderingHandlerVK::RenderingHandlerVK(GraphicsContextVK* pGraphicsContext)
    :m_pGraphicsContext(pGraphicsContext),
	m_pMeshRenderer(nullptr),
	m_pRaytracer(nullptr),
	m_pParticleRenderer(nullptr),
    m_pRenderPass(nullptr),
    m_CurrentFrame(0)
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_ppCommandPools[i] = VK_NULL_HANDLE;
        m_ppCommandBuffers[i] = VK_NULL_HANDLE;
    }
}

RenderingHandlerVK::~RenderingHandlerVK()
{
	SAFEDELETE(m_pRenderPass);
	releaseBackBuffers();

	VkDevice device = m_pGraphicsContext->getDevice()->getDevice();
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		SAFEDELETE(m_ppCommandPools[i]);

		if (m_ImageAvailableSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
		}

		if (m_RenderFinishedSemaphores[i] != VK_NULL_HANDLE) {
			vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
		}
    }
}

bool RenderingHandlerVK::initialize()
{
	if (!createRenderPass()) {
		return false;
	}

	if (!createBackBuffers()) {
		return false;
	}

	if (!createCommandPoolAndBuffers()) {
		return false;
	}

	if (!createSemaphores()) {
		return false;
	}

	return true;
}

void RenderingHandlerVK::onWindowResize(uint32_t width, uint32_t height)
{
	m_pGraphicsContext->getDevice()->wait();
	releaseBackBuffers();

	m_pGraphicsContext->getSwapChain()->resize(width, height);

	createBackBuffers();
}

void RenderingHandlerVK::beginFrame(const Camera& camera)
{
    // Prepare for frame
	//m_pGraphicsContext->getSwapChain()->acquireNextImage(m_ImageAvailableSemaphores[m_CurrentFrame]);
	//uint32_t backBufferIndex = m_pGraphicsContext->getSwapChain()->getImageIndex();

	std::vector<std::thread> recordingThreads;

    if (m_pMeshRenderer != nullptr) {
        recordingThreads.push_back(std::thread(&IRenderer::beginFrame, m_pMeshRenderer, camera));
    }

    if (m_pRaytracer != nullptr) {
        recordingThreads.push_back(std::thread(&IRenderer::beginFrame, m_pRaytracer, camera));
    }

    if (m_pParticleRenderer != nullptr) {
        recordingThreads.push_back(std::thread(&IRenderer::beginFrame, m_pParticleRenderer, camera));
    }

    for (std::thread& thread : recordingThreads) {
        thread.join();
    }

	recordingThreads.clear();
}

void RenderingHandlerVK::endFrame()
{
    if (m_pMeshRenderer != nullptr) {
        m_pMeshRenderer->endFrame();
    }

    if (m_pRaytracer != nullptr) {
        m_pRaytracer->endFrame();
    }

    if (m_pParticleRenderer != nullptr) {
        m_pParticleRenderer->endFrame();
    }

    // Submit the rendering handler's command buffer
    /*m_ppCommandBuffers[m_CurrentFrame]->endRenderPass();
	m_ppCommandBuffers[m_CurrentFrame]->end();

	//Execute commandbuffer
	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	DeviceVK* pDevice = m_pGraphicsContext->getDevice();
	pDevice->executeCommandBuffer(pDevice->getGraphicsQueue(), m_ppCommandBuffers[m_CurrentFrame], waitSemaphores, waitStages, 1, signalSemaphores, 1);*/
}

void RenderingHandlerVK::swapBuffers()
{
    m_pGraphicsContext->swapBuffers(m_RenderFinishedSemaphores[m_CurrentFrame]);
	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RenderingHandlerVK::drawImgui(IImgui* pImgui)
{
	m_pMeshRenderer->drawImgui(pImgui);
    //pImgui->render(m_ppCommandBuffers[m_CurrentFrame]); TODO: Get this running
}

void RenderingHandlerVK::setMeshRenderer(IRenderer* pMeshRenderer)
{
	m_pMeshRenderer = pMeshRenderer;
}

void RenderingHandlerVK::setRaytracer(IRenderer* pRaytracer)
{
	m_pRaytracer = pRaytracer;
}

void RenderingHandlerVK::setParticleRenderer(IRenderer* pParticleRenderer)
{
	m_pParticleRenderer = pParticleRenderer;
}

bool RenderingHandlerVK::createBackBuffers()
{
    SwapChainVK* pSwapChain = m_pGraphicsContext->getSwapChain();
	DeviceVK* pDevice = m_pGraphicsContext->getDevice();

	VkExtent2D extent = pSwapChain->getExtent();
	ImageViewVK* pDepthStencilView = pSwapChain->getDepthStencilView();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_ppBackbuffers[i] = DBG_NEW FrameBufferVK(pDevice);
		m_ppBackbuffers[i]->addColorAttachment(pSwapChain->getImageView(i));
		m_ppBackbuffers[i]->setDepthStencilAttachment(pDepthStencilView);
		if (!m_ppBackbuffers[i]->finalize(m_pRenderPass, extent.width, extent.height)) {
			return false;
		}
	}

	return true;
}

bool RenderingHandlerVK::createCommandPoolAndBuffers()
{
    DeviceVK* pDevice = m_pGraphicsContext->getDevice();
    const uint32_t queueFamilyIndices = pDevice->getQueueFamilyIndices().graphicsFamily.value();

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_ppCommandPools[i] = DBG_NEW CommandPoolVK(pDevice, queueFamilyIndices);
		if (!m_ppCommandPools[i]->init()) {
			return false;
		}

        m_ppCommandBuffers[i] = m_ppCommandPools[i]->allocateCommandBuffer();
        if (m_ppCommandBuffers[i] == nullptr) {
            return false;
        }
    }
}

bool RenderingHandlerVK::createRenderPass()
{
	//Create renderpass
	m_pRenderPass = DBG_NEW RenderPassVK(m_pGraphicsContext->getDevice());
	VkAttachmentDescription description = {};
	description.format = VK_FORMAT_B8G8R8A8_UNORM;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				//Clear Before Rendering
	description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				//Store After Rendering
	description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	//Dont care about stencil before
	description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	//Still dont care about stencil
	description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			//Dont care about what the initial layout of the image is (We do not need to save this memory)
	description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		//The image will be used for presenting
	m_pRenderPass->addAttachment(description);

	description.format = VK_FORMAT_D24_UNORM_S8_UINT;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				//Clear Before Rendering
	description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				//Store After Rendering
	description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	//Dont care about stencil before (If we need the stencil this needs to change)
	description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	//Still dont care about stencil	 (If we need the stencil this needs to change)
	description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;							//Dont care about what the initial layout of the image is (We do not need to save this memory)
	description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;		//The image will be used as depthstencil
	m_pRenderPass->addAttachment(description);

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentRef = {};
	depthStencilAttachmentRef.attachment = 1;
	depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pRenderPass->addSubpass(&colorAttachmentRef, 1, &depthStencilAttachmentRef);
	m_pRenderPass->addSubpassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
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

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]), "Failed to create semaphores for a Frame");
		VK_CHECK_RESULT_RETURN_FALSE(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]), "Failed to create semaphores for a Frame");
	}

	return true;
}

void RenderingHandlerVK::releaseBackBuffers()
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		SAFEDELETE(m_ppBackbuffers[i]);
	}
}
