#include "VulkanRenderer.h"

#include "VulkanMaterial.h"
#include "VulkanRenderState.h"
#include "VulkanConstantBuffer.h"
#include "VulkanSampler2D.h"
#include "VulkanTexture2D.h"
#include "VulkanVertexBuffer.h"

#include "../Mesh.h"
#include "../IA.h"

#include <cassert>

VulkanRenderer::VulkanRenderer()
	: m_FrameIndex(0),
	m_pDescriptorData(nullptr)
{
	memset(m_pConstantBuffer, 0, sizeof(m_pConstantBuffer));
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
	return new Mesh();
}

VertexBuffer* VulkanRenderer::makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage)
{
	VulkanVertexBuffer* pVertexBuffer = new VulkanVertexBuffer(this, size, usage);
	m_VertexBuffers.push_back(pVertexBuffer);
	return pVertexBuffer;
}

Texture2D* VulkanRenderer::makeTexture2D()
{
	return new VulkanTexture2D(this);
}

Sampler2D* VulkanRenderer::makeSampler2D()
{
	return new VulkanSampler2D(&m_VulkanDevice);
}

RenderState* VulkanRenderer::makeRenderState()
{
	VulkanRenderState* pRenderState = new VulkanRenderState(&m_VulkanDevice);
	m_RenderStates.push_back(pRenderState);
	return pRenderState;
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
	VulkanConstantBuffer* pConstantBuffer = new VulkanConstantBuffer(NAME, location, m_FrameIndex);
	pConstantBuffer->provideResources(this, &m_VulkanDevice);
	m_ConstantBuffers.push_back(pConstantBuffer);
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

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) 
	{
		std::cout << "Failed to create Buffer" << std::endl;
	}
	else
	{
		std::cout << "Created buffer" << std::endl;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(m_VulkanDevice.getPhysicalDevice(), memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		std::cout << "Failed to allocate buffermemory" << std::endl;
	}
	else
	{
		std::cout << "Allocated buffermemory" << std::endl;
	}

	vkBindBufferMemory(device, buffer, bufferMemory, memoryOffset);
}

void VulkanRenderer::setVertexBuffer(VulkanVertexBuffer* pBuffer, uint32_t slot)
{
	m_pVertexBuffers[slot] = pBuffer;
}

void VulkanRenderer::setConstantBuffer(VulkanConstantBuffer* pBuffer, uint32_t slot)
{
	m_pConstantBuffer[slot] = (pBuffer != nullptr) ? pBuffer : m_pDefaultConstantBuffer;
}

void VulkanRenderer::setTexture2D(VulkanTexture2D* pTexture2D, VulkanSampler2D* pSampler2D)
{
	m_pTexture2D = (pTexture2D != nullptr) ? pTexture2D : m_pDefaultTexture2D;
	m_pSampler2D = (pSampler2D != nullptr) ? pSampler2D : m_pDefaultSampler2D;
}

void VulkanRenderer::commitState()
{
	updateVertexBufferDescriptorSets();
	updateConstantBufferDescriptorSets();
	updateSamplerDescriptorSets();
}

void VulkanRenderer::createImage(VkImage& image, VkDeviceMemory& imageMemory, unsigned int width, unsigned int height, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.flags = 0;
	imageInfo.extent.depth = 1;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.format = format;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.mipLevels = 1;
	imageInfo.pNext = nullptr;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = usage;
	imageInfo.arrayLayers = 1;

	VkDevice device = m_VulkanDevice.getDevice();

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(m_VulkanDevice.getPhysicalDevice(), memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

//VulkanCommandBuffer* VulkanRenderer::beginSingleTimeCommands()
//{
//	VulkanCommandBuffer* commandBuffer = new VulkanCommandBuffer();
//	commandBuffer->initialize(&m_VulkanDevice);
//
//	VkCommandBufferBeginInfo beginInfo = {};
//	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//
//	vkBeginCommandBuffer(commandBuffer->getCommandBuffer(), &beginInfo);
//
//	return commandBuffer;
//}
//
//void VulkanRenderer::endSingleTimeCommands(VulkanCommandBuffer* commandBuffer)
//{
//	VkCommandBuffer cmdBuffer = commandBuffer->getCommandBuffer();
//
//	vkEndCommandBuffer(cmdBuffer);
//
//	VkSubmitInfo submitInfo = {};
//	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//	submitInfo.commandBufferCount = 1;
//	submitInfo.pCommandBuffers = &cmdBuffer;
//
//	VkQueue graphicsQueue = m_VulkanDevice.getGraphicsQueue();
//
//	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
//	vkQueueWaitIdle(graphicsQueue);
//
//	delete commandBuffer;
//}

void VulkanRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	//VulkanCommandBuffer* commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	//vkCmdPipelineBarrier(commandBuffer->getCommandBuffer(),	sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	//endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	//VulkanCommandBuffer* commandBuffer = this->beginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferImageHeight = 0;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.depth = 1;
	region.imageExtent.height = height;
	region.imageExtent.width = width;

	//vkCmdCopyBufferToImage(commandBuffer->getCommandBuffer(), buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	//this->endSingleTimeCommands(commandBuffer);
}

int VulkanRenderer::createImageView(VkImageView& imageView, VkImage image, VkFormat format)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.pNext = nullptr;
	viewInfo.image = image;
	viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
	viewInfo.flags = 0;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

	if (vkCreateImageView(m_VulkanDevice.getDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image view!");
		return -1;
	}

	return 0;
}

int VulkanRenderer::createTexture(VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory, const void* pPixels, uint32_t width, uint32_t height)
{
    VkDeviceSize imageSize = width * height * 4;

	// Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    this->createBuffer(stagingBuffer, stagingBufferMemory, imageSize, 0,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

	VkDevice device = m_VulkanDevice.getDevice();

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pPixels, imageSize);
	vkUnmapMemory(device, stagingBufferMemory);

	// Transfer staging buffer contents to the final texture buffer
	createImage(image, imageMemory, width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, image, width, height);
	transitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	return createImageView(imageView, image, VK_FORMAT_R8G8B8A8_UNORM);
}

int VulkanRenderer::initialize(unsigned width, unsigned height)
{
	/*if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "%s", SDL_GetError());
		exit(-1);
	}

	m_pWindow = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);*/
	
	//m_VulkanDevice.initialize("Prankster", MAX_NUM_STORAGE_DESCRIPTORS, MAX_NUM_UNIFORM_DESCRIPTORS, MAX_NUM_SAMPLER_DESCRIPTORS, MAX_NUM_DESCRIPTOR_SETS);

	initializeRenderPass();

	createSemaphores();
	
	//m_Swapchain.initialize(&m_VulkanDevice, m_pWindow, m_RenderPass, VK_FORMAT_B8G8R8A8_UNORM, MAX_FRAMES_IN_FLIGHT, true);

	/*for (auto& commandBuffer : m_VulkanCommandBuffers) {
		commandBuffer.initialize(&m_VulkanDevice);
	}*/

	DescriptorSetLayouts descriptorSetLayouts = {};
	VkPipelineLayout pipelineLayout = {};
	VkDescriptorSet descriptorSets[5/*DESCRIPTOR_SETS_PER_TRIANGLE*/];
	memset(descriptorSets, 0, sizeof(descriptorSets));

	createDescriptorSetLayouts(descriptorSetLayouts);
	createPipelineLayout(pipelineLayout, descriptorSetLayouts);
	createDescriptorSets(descriptorSets, descriptorSetLayouts);

	m_pDescriptorData = new DescriptorData();
	m_pDescriptorData->pipelineLayout = pipelineLayout;
	m_pDescriptorData->descriptorSetLayouts = descriptorSetLayouts;
	memcpy(m_pDescriptorData->descriptorSets, descriptorSets, sizeof(descriptorSets));

	m_pDefaultConstantBuffer = reinterpret_cast<VulkanConstantBuffer*>(makeConstantBuffer("NULL BUFFER", 0));
	m_pDefaultConstantBuffer->initialize(4);
	for (uint32_t i = 0; i < 7; i++)
		m_pConstantBuffer[i] = m_pDefaultConstantBuffer;

	m_pDefaultSampler2D = reinterpret_cast<VulkanSampler2D*>(makeSampler2D());
	m_pDefaultSampler2D->setWrap(WRAPPING::REPEAT, WRAPPING::REPEAT);
	m_pDefaultSampler2D->setMinFilter(FILTER::POINT_SAMPLER);
	m_pDefaultSampler2D->setMagFilter(FILTER::POINT_SAMPLER);
	m_pSampler2D = m_pDefaultSampler2D;

	uint8_t pixels[] = { 255, 255, 255, 255 };
	m_pDefaultTexture2D = reinterpret_cast<VulkanTexture2D*>(makeTexture2D());
	m_pDefaultTexture2D->loadFromMemory(pixels, 1, 1);
	m_pTexture2D = m_pDefaultTexture2D;
	return 0;
}

void VulkanRenderer::setWinTitle(const char* title)
{
	//SDL_SetWindowTitle(m_pWindow, title);
}

void VulkanRenderer::present()
{
	//m_Swapchain.present(m_RenderFinishedSemaphores[m_FrameIndex]);
	m_FrameIndex = (m_FrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

	for (VulkanConstantBuffer* pConstantBuffer : m_ConstantBuffers) {
		pConstantBuffer->setCurrentFrame(m_FrameIndex);
	}
}

int VulkanRenderer::shutdown()
{
	if (m_VulkanDevice.getDevice() != VK_NULL_HANDLE)
		vkDeviceWaitIdle(m_VulkanDevice.getDevice());

	delete m_pDefaultTexture2D;
	delete m_pDefaultSampler2D;

	for (auto constantBuffer : m_ConstantBuffers)
		delete constantBuffer;

	m_ConstantBuffers.clear();

	for (auto vertexBuffer : m_VertexBuffers)
		delete vertexBuffer;
	
	m_VertexBuffers.clear();
	
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

		//m_VulkanCommandBuffers[i].release();
	}

	vkDestroyRenderPass(m_VulkanDevice.getDevice(), m_RenderPass, nullptr);

	for (VulkanRenderState* pRenderState : m_RenderStates) {
		delete pRenderState;
	}

	//m_Swapchain.release();
	m_VulkanDevice.release();

	//SDL_DestroyWindow(m_pWindow);

	delete this;
	return 0;
}

void VulkanRenderer::setClearColor(float r, float g, float b, float a)
{
	m_ClearColor.color.float32[0] = r;
	m_ClearColor.color.float32[1] = g;
	m_ClearColor.color.float32[2] = b;
	m_ClearColor.color.float32[3] = a;
}

void VulkanRenderer::clearBuffer(unsigned)
{
	//Leave Empty
}

void VulkanRenderer::setRenderState(RenderState* ps)
{
	//m_VulkanCommandBuffers[m_FrameIndex].bindPipelineState(reinterpret_cast<VulkanRenderState*>(ps)->getPipelineState());
}

void VulkanRenderer::submit(Mesh* mesh)
{
	m_DrawList[mesh->technique].push_back(mesh);
}

void VulkanRenderer::frame()
{
	//m_Swapchain.acquireNextImage(m_ImageAvailableSemaphores[m_FrameIndex]);

	//m_VulkanCommandBuffers[m_FrameIndex].reset();
	m_VulkanDevice.cleanDescriptorPools(m_FrameIndex);
	//m_VulkanCommandBuffers[m_FrameIndex].beginCommandBuffer();

	for (auto& constantBuffer : m_ConstantBuffers)
	{
		//constantBuffer->copyToBuffer(&m_VulkanCommandBuffers[m_FrameIndex]);
	}
	
	VkClearValue depthClear = {};
	depthClear.depthStencil.depth = 1.0f;
	depthClear.depthStencil.stencil = 0;

	VkClearValue clearValues[] = { m_ClearColor, depthClear };
	//m_VulkanCommandBuffers[m_FrameIndex].beginRenderPass(m_RenderPass, m_Swapchain.getCurrentFramebuffer(), m_Swapchain.getExtent(), clearValues, 2);
	
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	//viewport.width = m_Swapchain.getExtent().width;
	//viewport.height = m_Swapchain.getExtent().height;
	//m_VulkanCommandBuffers[m_FrameIndex].setViewport(viewport);

	VkRect2D scissor = {};
	//scissor.extent = m_Swapchain.getExtent();
	scissor.offset = { 0, 0 };
	//m_VulkanCommandBuffers[m_FrameIndex].setScissor(scissor);

	for (auto& drawlist : m_DrawList)
	{
		Technique* pTechnique = drawlist.first;
		pTechnique->enable(this);

		for (auto mesh : drawlist.second)
		{
			size_t numberElements = mesh->geometryBuffers[0].numElements;
		
			for (auto t : mesh->textures)
			{
				// we do not really know here if the sampler has been
				// defined in the shader.
				t.second->bind(t.first);
			}
			for (auto element : mesh->geometryBuffers) 
			{
				mesh->bindIAVertexBuffer(element.first);
			}
			mesh->txBuffer->bind(mesh->technique->getMaterial());
			
			VkDescriptorSet* descriptorSets = m_pDescriptorData->descriptorSets + (m_FrameIndex * 2);
			allocateFrameDescriptors(descriptorSets, m_pDescriptorData->descriptorSetLayouts);
			
			this->commitState();

			//m_VulkanCommandBuffers[m_FrameIndex].bindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_pDescriptorData->pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
			//m_VulkanCommandBuffers[m_FrameIndex].drawInstanced(numberElements, 1, 0, 0);
		}
	}
	m_DrawList.clear();
	
	//m_VulkanCommandBuffers[m_FrameIndex].endRenderPass();
	//m_VulkanCommandBuffers[m_FrameIndex].endCommandBuffer();


	//Submit
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;

	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_FrameIndex] };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = waitStages;

	VkCommandBuffer commandBuffers[] = { VK_NULL_HANDLE /*m_VulkanCommandBuffers[m_FrameIndex].getCommandBuffer()*/ };
	submitInfo.pCommandBuffers = commandBuffers;
	submitInfo.commandBufferCount = 1;

	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_FrameIndex] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VkResult result = vkQueueSubmit(m_VulkanDevice.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE /*m_VulkanCommandBuffers[m_FrameIndex].getFence()*/);
	if (result != VK_SUCCESS)
	{
		std::cout << "vkQueueSubmit failed. Error: " << result << std::endl;
	}
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
    uniformLayoutInfo.bindingCount = 0; //ARRAYSIZE(descriptorSetLayoutBindings);
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
    pipelineLayoutInfo.setLayoutCount = 0;//ARRAYSIZE(descriptorSetLayouts.layouts);
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
	m_AllocatedSets[0] = 2;
	m_AllocatedSets[1] = 2;
	m_AllocatedSets[2] = 2;
	
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorSetLayout allLayouts[] =
		{
			descriptorSetLayouts.vertexAndConstantBufferDescriptorSetLayout, //Frame 0
			descriptorSetLayouts.textureDescriptorSetLayout,
		};
		uint32_t count = 0;//ARRAYSIZE(allLayouts);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = m_VulkanDevice.getDescriptorPool(i);
		allocInfo.descriptorSetCount = count;
		allocInfo.pSetLayouts = allLayouts;

		if (vkAllocateDescriptorSets(m_VulkanDevice.getDevice(), &allocInfo, &descriptorSets[i * count]) != VK_SUCCESS)
		{
			std::cout << "Failed to allocate DescriptorSets" << std::endl;
		}
		else
		{
			m_AllocatedSets[i]++;
			//std::cout << "Allocated DescriptorSets " << m_AllocatedSets[i] << std::endl;
		}
	}
}

void VulkanRenderer::allocateFrameDescriptors(VkDescriptorSet descriptorSets[], DescriptorSetLayouts& descriptorSetLayouts)
{
	VkDescriptorSetLayout allLayouts[] =
	{
		descriptorSetLayouts.vertexAndConstantBufferDescriptorSetLayout, //Frame 0
		descriptorSetLayouts.textureDescriptorSetLayout,
	};
	uint32_t count = 0;//ARRAYSIZE(allLayouts);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = m_VulkanDevice.getDescriptorPool(m_FrameIndex);
	allocInfo.descriptorSetCount = count;
	allocInfo.pSetLayouts = allLayouts;

	if (vkAllocateDescriptorSets(m_VulkanDevice.getDevice(), &allocInfo, descriptorSets) != VK_SUCCESS)
	{
		std::cout << "Failed to allocate DescriptorSets" << std::endl;
	}
	else
	{
		m_AllocatedSets[m_FrameIndex]++;
		//std::cout << "Allocated DescriptorSets " << m_AllocatedSets[m_FrameIndex] << " on frame " << m_FrameIndex << std::endl;
	}

	if (m_AllocatedSets[m_FrameIndex] >= (5/*MAX_NUM_DESCRIPTOR_SETS*/ / 2) - 3)
	{
		m_VulkanDevice.reallocDescriptorPool(m_FrameIndex);
		m_AllocatedSets[m_FrameIndex] = 0;
	}
}

void VulkanRenderer::updateVertexBufferDescriptorSets()
{
	for (size_t i = 0; i < 5/*VERTEX_BUFFER_DESCRIPTORS_PER_SET_BUNDLE*/; i++)
	{
		//Cache buffer
		VulkanVertexBuffer* pBuffer = m_pVertexBuffers[i];
		assert(pBuffer != nullptr);

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer	= pBuffer->getBuffer();
		bufferInfo.offset	= pBuffer->getOffset();
		bufferInfo.range	= pBuffer->getBoundSize();

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
	for (size_t i = 0; i < 5/*CONSTANT_BUFFER_DESCRIPTORS_PER_SET_BUNDLE*/; i++)
	{
		VulkanConstantBuffer* pBuffer = m_pConstantBuffer[5 + i];
		assert(pBuffer != nullptr);

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer	= pBuffer->getBuffer();
		bufferInfo.offset	= 0;
		bufferInfo.range	= VK_WHOLE_SIZE;

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
	assert(m_pTexture2D != nullptr);
	assert(m_pSampler2D != nullptr);

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageView		= m_pTexture2D->getImageView();
	imageInfo.sampler		= m_pSampler2D->getSampler();
	imageInfo.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet descriptorImageWrite = {};
	descriptorImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorImageWrite.dstSet = m_pDescriptorData->descriptorSets[2 * m_FrameIndex + 1];
	descriptorImageWrite.dstBinding = 0;
	descriptorImageWrite.dstArrayElement = 0;
	descriptorImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorImageWrite.descriptorCount = 1;
	descriptorImageWrite.pBufferInfo = nullptr;
	descriptorImageWrite.pImageInfo = &imageInfo;
	descriptorImageWrite.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(m_VulkanDevice.getDevice(), 1, &descriptorImageWrite, 0, nullptr);
}
