#include "VulkanRenderer.h"
#include "VulkanMaterial.h"
#include "VulkanRenderState.h"

VulkanRenderer::VulkanRenderer()
	: m_SemaphoreIndex(0)
{
}

VulkanRenderer::~VulkanRenderer()
{
}

Material* VulkanRenderer::makeMaterial(const std::string& name)
{
	return new VulkanMaterial(this, &m_VulkanDevice, name);
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
	return new VulkanRenderState(&m_VulkanDevice);
}

std::string VulkanRenderer::getShaderPath()
{
	return std::string("..\\assets\\Vulkan\\");
}

std::string VulkanRenderer::getShaderExtension()
{
	return std::string(".glsl");
}

ConstantBuffer* VulkanRenderer::makeConstantBuffer(std::string NAME, unsigned location)
{
	return nullptr;
}

Technique* VulkanRenderer::makeTechnique(Material* pMaterial, RenderState* pRenderState)
{
	VulkanMaterial* pVkMaterial = reinterpret_cast<VulkanMaterial*>(pMaterial);
	pVkMaterial->finalize();

	reinterpret_cast<VulkanRenderState*>(pRenderState)->finalize(pVkMaterial, m_RenderPass);
	return new Technique(pMaterial, pRenderState);
}

void VulkanRenderer::createBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize size, VkDeviceSize memoryOffset, VkMemoryPropertyFlags properties, VkBufferUsageFlags usage)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.flags = 0;
	bufferInfo.size = size;
	bufferInfo.pNext = nullptr;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.usage = usage;

	uint32_t graphicsFamilyIndex = m_VulkanDevice.getQueueFamilyIndices().graphicsFamily.value();
	bufferInfo.pQueueFamilyIndices = &graphicsFamilyIndex;
	bufferInfo.queueFamilyIndexCount = 1;

	VkDevice device = m_VulkanDevice.getDevice();

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Render Pass!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(m_VulkanDevice.getPhysicalDevice(), memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(device, buffer, bufferMemory, memoryOffset);
}

int VulkanRenderer::initialize(unsigned width, unsigned height)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "%s", SDL_GetError());
		exit(-1);
	}

	m_pWindow = SDL_CreateWindow("OpenGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
	
	m_VulkanDevice.initialize("Prankster", MAX_NUM_UNIFORM_DESCRIPTORS, MAX_NUM_SAMPLER_DESCRIPTORS, MAX_NUM_DESCRIPTOR_SETS);

	initializeRenderPass();

	createSemaphores();
	
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

		m_VulkanCommandBuffers[i].release();
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

	VkAttachmentDescription depthStencilAttachment = {};
	depthStencilAttachment.format = VK_FORMAT_D24_UNORM_S8_UINT;
	depthStencilAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				//Clear Before Rendering
	depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				//Store After Rendering

	depthStencilAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	//Dont care about stencil before
	depthStencilAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	//Still dont care about stencil

	depthStencilAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			//Dont care about what the initial layout of the image is
	depthStencilAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;		//The image will be used for presenting

	VkAttachmentReference depthStencilAttachmentRef = {};
	depthStencilAttachmentRef.attachment = 1;
	depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthStencilAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription attachmentDescriptions[] = { colorAttachment, depthStencilAttachment };
	
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachmentDescriptions;
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
