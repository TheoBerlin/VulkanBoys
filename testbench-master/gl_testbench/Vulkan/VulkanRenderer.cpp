#include "VulkanRenderer.h"
#include "VulkanMaterial.h"
#include "VulkanRenderState.h"
#include "VulkanConstantBuffer.h"
#include "VulkanSampler2D.h"

#include "../IA.h"

VulkanRenderer::VulkanRenderer()
	: m_FrameIndex(0),
	m_pDescriptorData(nullptr)
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
	return new VulkanSampler2D(&m_VulkanDevice);
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
	VulkanConstantBuffer* pConstantBuffer = new VulkanConstantBuffer(NAME, location);
	pConstantBuffer->provideResources(this, &m_VulkanDevice);
	return pConstantBuffer;
}

Technique* VulkanRenderer::makeTechnique(Material* pMaterial, RenderState* pRenderState)
{
	VulkanMaterial* pVkMaterial = reinterpret_cast<VulkanMaterial*>(pMaterial);
	reinterpret_cast<VulkanRenderState*>(pRenderState)->finalize(pVkMaterial, m_RenderPass, m_pDescriptorData->pipelineLayout);
	
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

void VulkanRenderer::setVertexBuffer(VulkanVertexBuffer* pBuffer, uint32_t slot)
{
	m_pVertexBuffers[slot] = pBuffer;
}

void VulkanRenderer::setConstantBuffer(VulkanConstantBuffer* pBuffer, uint32_t slot)
{
	m_pConstantBuffer[slot] = pBuffer;
}

void VulkanRenderer::setTexture2D(VulkanTexture2D* pTexture2D, VulkanSampler2D* pSampler2D)
{
	m_pTexture2D = pTexture2D;
	m_pSampler2D = pSampler2D;
}

void VulkanRenderer::commitState()
{
	updateVertexBufferDescriptorSets();
	updateConstantBufferDescriptorSets();
	updateSamplerDescriptorSets();
}


int VulkanRenderer::initialize(unsigned width, unsigned height)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "%s", SDL_GetError());
		exit(-1);
	}

	m_pWindow = SDL_CreateWindow("OpenGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
	
	m_VulkanDevice.initialize("Prankster", MAX_NUM_STORAGE_DESCRIPTORS, MAX_NUM_UNIFORM_DESCRIPTORS, MAX_NUM_SAMPLER_DESCRIPTORS, MAX_NUM_DESCRIPTOR_SETS);

	initializeRenderPass();

	createSemaphores();
	
	m_Swapchain.initialize(&m_VulkanDevice, m_pWindow, m_RenderPass, VK_FORMAT_B8G8R8A8_UNORM, MAX_FRAMES_IN_FLIGHT, true);

	for (auto& commandBuffer : m_VulkanCommandBuffers) {
		commandBuffer.initialize(&m_VulkanDevice);
	}

	DescriptorSetLayouts descriptorSetLayouts = {};
	VkPipelineLayout pipelineLayout = {};
	VkDescriptorSet descriptorSets[DESCRIPTOR_SETS_PER_TRIANGLE];

	createDescriptorSetLayouts(descriptorSetLayouts);
	createPipelineLayout(pipelineLayout, descriptorSetLayouts);
	createDescriptorSets(descriptorSets, descriptorSetLayouts);

	m_pDescriptorData = new DescriptorData();
	m_pDescriptorData->pipelineLayout = pipelineLayout;
	m_pDescriptorData->descriptorSetLayouts = descriptorSetLayouts;
	m_pDescriptorData->descriptorSets = descriptorSets;

	return 0;
}

void VulkanRenderer::setWinTitle(const char* title)
{
	SDL_SetWindowTitle(m_pWindow, title);
}

void VulkanRenderer::present()
{
	m_Swapchain.present(m_RenderFinishedSemaphores[m_FrameIndex]);
	m_FrameIndex = (m_FrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

int VulkanRenderer::shutdown()
{
	if (m_pDescriptorData != nullptr)
	{
		vkDestroyDescriptorSetLayout(m_VulkanDevice.getDevice(), m_pDescriptorData->descriptorSetLayouts.vertexAndConstantBufferDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_VulkanDevice.getDevice(), m_pDescriptorData->descriptorSetLayouts.textureDescriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(m_VulkanDevice.getDevice(), m_pDescriptorData->pipelineLayout, nullptr);
		
		delete m_pDescriptorData;
		m_pDescriptorData = nullptr;
	}
	
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

void VulkanRenderer::setClearColor(float r, float g, float b, float a)
{
	m_ClearColor.r = r;
	m_ClearColor.g = g;
	m_ClearColor.b = b;
	m_ClearColor.a = a;
}

void VulkanRenderer::clearBuffer(unsigned)
{
	//Leave Empty
}

void VulkanRenderer::setRenderState(RenderState* ps)
{
	m_VulkanCommandBuffers[m_FrameIndex].setPipelineState(reinterpret_cast<VulkanRenderState*>(ps)->getPipelineState());
}

void VulkanRenderer::submit(Mesh* mesh)
{
	m_DrawList[mesh->technique].push_back(mesh);
}

void VulkanRenderer::frame()
{
	m_Swapchain.acquireNextImage(m_ImageAvailableSemaphores[m_FrameIndex]);

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

void VulkanRenderer::createDescriptorSetLayouts(DescriptorSetLayouts& descriptorSetLayouts)
{
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[5];

	descriptorSetLayoutBindings[0].binding = POSITION;
	descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorSetLayoutBindings[0].descriptorCount = 1;
	descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;

	descriptorSetLayoutBindings[1].binding = NORMAL;
	descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorSetLayoutBindings[1].descriptorCount = 1;
	descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;

	descriptorSetLayoutBindings[2].binding = TEXTCOORD;
	descriptorSetLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorSetLayoutBindings[2].descriptorCount = 1;
	descriptorSetLayoutBindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutBindings[2].pImmutableSamplers = nullptr;

	descriptorSetLayoutBindings[3].binding = TRANSLATION;
	descriptorSetLayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetLayoutBindings[3].descriptorCount = 1;
	descriptorSetLayoutBindings[3].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutBindings[3].pImmutableSamplers = nullptr;

	descriptorSetLayoutBindings[4].binding = DIFFUSE_TINT;
	descriptorSetLayoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetLayoutBindings[4].descriptorCount = 1;
	descriptorSetLayoutBindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutBindings[4].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo uniformLayoutInfo = {};
	uniformLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	uniformLayoutInfo.bindingCount = ARRAYSIZE(descriptorSetLayoutBindings);
	uniformLayoutInfo.pBindings = descriptorSetLayoutBindings;

	if (vkCreateDescriptorSetLayout(m_VulkanDevice.getDevice(), &uniformLayoutInfo, nullptr, &descriptorSetLayouts.vertexAndConstantBufferDescriptorSetLayout) != VK_SUCCESS)
	{
		std::cout << "Failed to create UniformDescriptorSetLayout" << std::endl;
	}
	else
	{
		std::cout << "Created UniformDescriptorSetLayout" << std::endl;
	}

	VkDescriptorSetLayoutBinding textureLayoutBinding = {};
	textureLayoutBinding.binding = 0;
	textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureLayoutBinding.descriptorCount = 1;
	textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	textureLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo textureLayoutInfo = {};
	textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textureLayoutInfo.bindingCount = 1;
	textureLayoutInfo.pBindings = &textureLayoutBinding;

	if (vkCreateDescriptorSetLayout(m_VulkanDevice.getDevice(), &textureLayoutInfo, nullptr, &descriptorSetLayouts.textureDescriptorSetLayout) != VK_SUCCESS)
	{
		std::cout << "Failed to create TextureDescriptorSetLayout" << std::endl;
	}
	else
	{
		std::cout << "Created TextureDescriptorSetLayout" << std::endl;
	}
}

void VulkanRenderer::createPipelineLayout(VkPipelineLayout& pipelineLayout, DescriptorSetLayouts& descriptorSetLayouts)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = ARRAYSIZE(descriptorSetLayouts.layouts);
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.layouts;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(m_VulkanDevice.getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		std::cout << "Failed to create PipelineLayout" << std::endl;
	}
	else
	{
		std::cout << "Created PipelineLayout" << std::endl;
	}
}

void VulkanRenderer::createDescriptorSets(VkDescriptorSet descriptorSets[], DescriptorSetLayouts& descriptorSetLayouts)
{
	VkDescriptorSetLayout allLayouts[] =
	{
		descriptorSetLayouts.vertexAndConstantBufferDescriptorSetLayout, //Frame 0
		descriptorSetLayouts.textureDescriptorSetLayout,
		
		descriptorSetLayouts.vertexAndConstantBufferDescriptorSetLayout, //Frame 1
		descriptorSetLayouts.textureDescriptorSetLayout,
		
		descriptorSetLayouts.vertexAndConstantBufferDescriptorSetLayout, //Frame 2
		descriptorSetLayouts.textureDescriptorSetLayout
	};
	
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_VulkanDevice.getDescriptorPool();
	allocInfo.descriptorSetCount = ARRAYSIZE(allLayouts);
	allocInfo.pSetLayouts = allLayouts;

	if (vkAllocateDescriptorSets(m_VulkanDevice.getDevice(), &allocInfo, descriptorSets) != VK_SUCCESS)
	{
		std::cout << "Failed to allocate DescriptorSets" << std::endl;
	}
	else
	{
		std::cout << "Allocated DescriptorSets" << std::endl;
	}
}

void VulkanRenderer::updateVertexBufferDescriptorSets()
{
	for (size_t i = 0; i < VERTEX_BUFFER_DESCRIPTORS_PER_SET_BUNDLE; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = nullptr; //Todo: Fixa grejer
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet descriptorBufferWrite = {};
		descriptorBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorBufferWrite.dstSet = m_pDescriptorData->descriptorSets[2 * m_FrameIndex];
		descriptorBufferWrite.dstBinding = i;
		descriptorBufferWrite.dstArrayElement = 0;
		descriptorBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorBufferWrite.descriptorCount = 1;
		descriptorBufferWrite.pBufferInfo = &bufferInfo;
		descriptorBufferWrite.pImageInfo = nullptr;
		descriptorBufferWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(m_VulkanDevice.getDevice(), 1, &descriptorBufferWrite, 0, nullptr);
	}
}

void VulkanRenderer::updateConstantBufferDescriptorSets()
{
	for (size_t i = 0; i < CONSTANT_BUFFER_DESCRIPTORS_PER_SET_BUNDLE; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = m_pConstantBuffer[5 + i]->getBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet descriptorBufferWrite = {};
		descriptorBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorBufferWrite.dstSet = m_pDescriptorData->descriptorSets[2 * m_FrameIndex];
		descriptorBufferWrite.dstBinding = 5 + i;
		descriptorBufferWrite.dstArrayElement = 0;
		descriptorBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorBufferWrite.descriptorCount = 1;
		descriptorBufferWrite.pBufferInfo = &bufferInfo;
		descriptorBufferWrite.pImageInfo = nullptr;
		descriptorBufferWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(m_VulkanDevice.getDevice(), 1, &descriptorBufferWrite, 0, nullptr);
	}
}

void VulkanRenderer::updateSamplerDescriptorSets()
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageView = nullptr; //Todo: Fixa grejer
	imageInfo.sampler = nullptr; //Todo: Fixa grejer
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet descriptorImageWrite = {};
	descriptorImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorImageWrite.dstSet = m_pDescriptorData->descriptorSets[2 * m_FrameIndex + 1];
	descriptorImageWrite.dstBinding = 0;
	descriptorImageWrite.dstArrayElement = 0;
	descriptorImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorImageWrite.descriptorCount = 1;
	descriptorImageWrite.pBufferInfo = nullptr;
	descriptorImageWrite.pImageInfo = &imageInfo;
	descriptorImageWrite.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(m_VulkanDevice.getDevice(), 1, &descriptorImageWrite, 0, nullptr);
}
