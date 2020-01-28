#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer()
	: m_SemaphoreIndex(0)
{
}

VulkanRenderer::~VulkanRenderer()
{
}

Material* VulkanRenderer::makeMaterial(const std::string& name)
{
	return nullptr;
}

Mesh* VulkanRenderer::makeMesh()
{
	return nullptr;
}

VertexBuffer* VulkanRenderer::makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage)
{
	return nullptr;
}

Texture2D* VulkanRenderer::makeTexture2D()
{
	return nullptr;
}

Sampler2D* VulkanRenderer::makeSampler2D()
{
	return nullptr;
}

RenderState* VulkanRenderer::makeRenderState()
{
	return nullptr;
}

std::string VulkanRenderer::getShaderPath()
{
	return std::string("..\\assets\\GL45\\");
}

std::string VulkanRenderer::getShaderExtension()
{
	return std::string(".glsl");
}

ConstantBuffer* VulkanRenderer::makeConstantBuffer(std::string NAME, unsigned location)
{
	return nullptr;
}

Technique* VulkanRenderer::makeTechnique(Material*, RenderState*)
{
	return nullptr;
}

int VulkanRenderer::initialize(unsigned width, unsigned height)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "%s", SDL_GetError());
		exit(-1);
	}

	m_pWindow = SDL_CreateWindow("OpenGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
	
	m_VulkanDevice.initialize("Prankster");

	initializeRenderPass();

	m_Swapchain.initialize(&m_VulkanDevice, m_pWindow, m_RenderPass, VK_FORMAT_B8G8R8A8_UNORM, MAX_FRAMES_IN_FLIGHT, true);

	for (auto& commandBuffer : m_VulkanCommandBuffers) {
		commandBuffer.initialize(&m_VulkanDevice);
	}

	return 0;
}

void VulkanRenderer::setWinTitle(const char* title)
{
	SDL_SetWindowTitle(m_pWindow, title);
}

void VulkanRenderer::present()
{
	m_Swapchain.present(m_RenderFinishedSemaphores[m_SemaphoreIndex]);
	m_SemaphoreIndex = (m_SemaphoreIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

int VulkanRenderer::shutdown()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		vkDestroySemaphore(m_VulkanDevice.getDevice(), m_RenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_VulkanDevice.getDevice(), m_ImageAvailableSemaphores[i], nullptr);
	}
	
	vkDestroyRenderPass(m_VulkanDevice.getDevice(), m_RenderPass, nullptr);
	
	m_Swapchain.release();
	m_VulkanDevice.release();

	SDL_DestroyWindow(m_pWindow);

	delete this;
	return 0;
}

void VulkanRenderer::setClearColor(float, float, float, float)
{
}

void VulkanRenderer::clearBuffer(unsigned)
{
}

void VulkanRenderer::setRenderState(RenderState* ps)
{
}

void VulkanRenderer::submit(Mesh* mesh)
{
}

void VulkanRenderer::frame()
{
	m_Swapchain.acquireNextImage(m_ImageAvailableSemaphores[m_SemaphoreIndex]);

	//TODO: Draw shit here
}

void VulkanRenderer::initializeRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				//Clear Before Rendering
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				//Store After Rendering

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	//Dont care about stencil before
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	//Still dont care about stencil

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			//Dont care about what the initial layout of the image is
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		//The image will be used for presenting

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_VulkanDevice.getDevice(), &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Render Pass!");
}

void VulkanRenderer::createSemaphores()
{
	m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(m_VulkanDevice.getDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_VulkanDevice.getDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS)
		{

			throw std::runtime_error("Failed to create semaphores for a Frame!");
		}
	}
}
