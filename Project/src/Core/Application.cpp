#include "Application.h"
#include "IWindow.h"
#include "Common/IContext.h"
#include "Common/IShader.h"

#include "Vulkan/ContextVK.h"
#include "Vulkan/CommandPoolVK.h"
#include "Vulkan/RenderPassVK.h"
#include "Vulkan/DescriptorSetLayoutVK.h"

Application g_Application;

Application::Application()
	: m_pWindow(nullptr),
	m_IsRunning(false)
{
}

void Application::init()
{
	m_pWindow = IWindow::create("Hello Vulkan", 800, 600);
	if (m_pWindow)
	{
		m_pWindow->setEventHandler(this);
	}

	m_pIContext = IContext::create(m_pWindow, API::VULKAN);

	IShader* pVertexShader = m_pIContext->createShader();
	pVertexShader->loadFromFile(EShader::VERTEX_SHADER, "main", "assets/shaders/vertex.spv");
	pVertexShader->finalize();

	IShader* pPixelShader = m_pIContext->createShader();
	pPixelShader->loadFromFile(EShader::PIXEL_SHADER, "main", "assets/shaders/fragment.spv");
	pPixelShader->finalize();

	SAFEDELETE(pVertexShader);
	SAFEDELETE(pPixelShader);

	//Should we have ICommandBuffer? Or is commandbuffers internal i.e belongs in the renderer?
	DeviceVK* pDevice = reinterpret_cast<ContextVK*>(m_pIContext)->getDevice();
	uint32_t queueFamilyIndex = pDevice->getQueueFamilyIndices().graphicsFamily.value();

	CommandPoolVK* pCommandPool = new CommandPoolVK(pDevice, queueFamilyIndex);
	pCommandPool->init();
	CommandBufferVK* pCommandBuffer = pCommandPool->allocateCommandBuffer();

	RenderPassVK* pRenderPass = new RenderPassVK(pDevice);
	VkAttachmentDescription description = {};
	description.format	= VK_FORMAT_B8G8R8A8_UNORM;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;				//Clear Before Rendering
	description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				//Store After Rendering
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;	//Dont care about stencil before
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;	//Still dont care about stencil
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;			//Dont care about what the initial layout of the image is (We do not need to save this memory)
	description.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		//The image will be used for presenting
	pRenderPass->addAttachment(description);

	description.format	= VK_FORMAT_D24_UNORM_S8_UINT;
	description.samples = VK_SAMPLE_COUNT_1_BIT;
	description.loadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;				//Clear Before Rendering
	description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				//Store After Rendering
	description.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;	//Dont care about stencil before (If we need the stencil this needs to change)
	description.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;	//Still dont care about stencil	 (If we need the stencil this needs to change)
	description.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;							//Dont care about what the initial layout of the image is (We do not need to save this memory)
	description.finalLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;		//The image will be used as depthstencil
	pRenderPass->addAttachment(description);

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentRef = {};
	depthStencilAttachmentRef.attachment = 1;
	depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	pRenderPass->addSubpass(&colorAttachmentRef, 1, &depthStencilAttachmentRef);
	pRenderPass->addSubpassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	pRenderPass->finalize();

	DescriptorSetLayoutVK* pDescriptorLayout = new DescriptorSetLayoutVK(pDevice);
	pDescriptorLayout->addBindingStorageBuffer(VK_SHADER_STAGE_VERTEX_BIT, 0, 1); //Vertex
	pDescriptorLayout->addBindingUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, 1, 1); //Transform
	pDescriptorLayout->addBindingSampledImage(VK_SHADER_STAGE_FRAGMENT_BIT, nullptr, 2, 1); //texture
	pDescriptorLayout->finalizeLayout();

	pCommandPool->freeCommandBuffer(&pCommandBuffer);
	SAFEDELETE(pCommandPool);
	SAFEDELETE(pRenderPass);
}

void Application::run()
{
	m_IsRunning = true;
	
	while (m_IsRunning)
	{
		m_pWindow->peekEvents();

		update();
		render();
	}
}

void Application::release()
{
	SAFEDELETE(m_pWindow);
	SAFEDELETE(m_pIContext);
}

void Application::OnWindowResize(uint32_t width, uint32_t height)
{
	D_LOG("Resize w=%d h%d", width , height);
}

void Application::OnWindowClose()
{
	D_LOG("Window Closed");
	m_IsRunning = false;
}

Application& Application::getInstance()
{
	return g_Application;
}

void Application::update()
{
}

void Application::render()
{
}
