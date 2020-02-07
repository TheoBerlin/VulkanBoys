#include "RendererVK.h"
#include "ContextVK.h"
#include "SwapChainVK.h"
#include "RenderPassVK.h"
#include "CommandPoolVK.h"
#include "CommandBufferVK.h"

RendererVK::RendererVK(ContextVK* pContext)
	: m_pContext(pContext),
	m_ppCommandPools(),
	m_ppCommandBuffers(),
	m_pRenderPass(nullptr),
	m_ppBackbuffers(),
	m_pPipelineState(nullptr),
	m_ClearColor(),
	m_Viewport(),
	m_ScissorRect()
{
}

RendererVK::~RendererVK()
{
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppCommandPools[i]->freeCommandBuffer(&m_ppCommandBuffers[i]);
		SAFEDELETE(m_ppCommandPools[i]);
		SAFEDELETE(m_ppBackbuffers[i]);
	}

	SAFEDELETE(m_pRenderPass);
	SAFEDELETE(m_pPipelineState);
	m_pContext = nullptr;
}

bool RendererVK::init()
{
	DeviceVK* pDevice = m_pContext->getDevice();
	SwapChainVK* pSwapChain = m_pContext->getSwapChain();

	//Create commandbuffers and pools
	const uint32_t queueFamilyIndex = pDevice->getQueueFamilyIndices().graphicsFamily.value();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		m_ppCommandPools[i] = new CommandPoolVK(pDevice, queueFamilyIndex);
		m_ppCommandBuffers[i] = m_ppCommandPools[i]->allocateCommandBuffer();
	}

	//TODO: Create framebuffers

	//Create renderpass
	m_pRenderPass = new RenderPassVK(pDevice);
	VkAttachmentDescription description = {};
	description.format	= VK_FORMAT_B8G8R8A8_UNORM;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;				//Clear Before Rendering
	description.storeOp	= VK_ATTACHMENT_STORE_OP_STORE;				//Store After Rendering
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;	//Dont care about stencil before
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;	//Still dont care about stencil
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;			//Dont care about what the initial layout of the image is (We do not need to save this memory)
	description.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		//The image will be used for presenting
	m_pRenderPass->addAttachment(description);

	description.format	= VK_FORMAT_D24_UNORM_S8_UINT;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;				//Clear Before Rendering
	description.storeOp	= VK_ATTACHMENT_STORE_OP_STORE;				//Store After Rendering
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;	//Dont care about stencil before (If we need the stencil this needs to change)
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;	//Still dont care about stencil	 (If we need the stencil this needs to change)
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;							//Dont care about what the initial layout of the image is (We do not need to save this memory)
	description.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;		//The image will be used as depthstencil
	m_pRenderPass->addAttachment(description);

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentRef = {};
	depthStencilAttachmentRef.attachment = 1;
	depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_pRenderPass->addSubpass(&colorAttachmentRef, 1, &depthStencilAttachmentRef);
	m_pRenderPass->addSubpassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	m_pRenderPass->finalize();

	//TODO: Create pipelinestate
}

void RendererVK::beginFrame()
{
	m_ppCommandBuffers[0]->reset();
	m_ppCommandPools[0]->reset();

	m_ppCommandBuffers[0]->begin();

	m_ppCommandBuffers[0]->beginRenderPass(nullptr, nullptr, 0, 0, &m_ClearColor, 1);

	m_ppCommandBuffers[0]->setViewports(&m_Viewport, 1);
	m_ppCommandBuffers[0]->setScissorRects(&m_ScissorRect, 1);
}

void RendererVK::endFrame()
{
	m_ppCommandBuffers[0]->endRenderPass();
	m_ppCommandBuffers[0]->end();
}

void RendererVK::setClearColor(float r, float g, float b)
{
	m_ClearColor.color.float32[0] = r;
	m_ClearColor.color.float32[1] = g;
	m_ClearColor.color.float32[2] = b;
	m_ClearColor.color.float32[3] = 1.0f;
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

void RendererVK::drawTriangle()
{
	m_ppCommandBuffers[0]->drawInstanced(3, 1, 0, 0);
}
