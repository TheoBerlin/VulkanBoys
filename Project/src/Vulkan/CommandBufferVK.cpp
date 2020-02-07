#include "CommandBufferVK.h"
#include "ImageVK.h"
#include "StackVK.h"
#include "DeviceVK.h"
#include "BufferVK.h"
#include "RenderPassVK.h"
#include "PipelineVK.h"
#include "FrameBufferVK.h"

CommandBufferVK::CommandBufferVK(DeviceVK* pDevice, VkCommandBuffer commandBuffer)
	: m_pDevice(pDevice),
	m_CommandBuffer(commandBuffer),
	m_Fence(VK_NULL_HANDLE),
	m_DescriptorSets()
{
}

CommandBufferVK::~CommandBufferVK()
{
	if (m_Fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(m_pDevice->getDevice(), m_Fence, nullptr);
		m_Fence = VK_NULL_HANDLE;
	}

	SAFEDELETE(m_pStack);
	m_pDevice = nullptr;
}

bool CommandBufferVK::finalize()
{
	// Create fence
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateFence(m_pDevice->getDevice(), &fenceInfo, nullptr, &m_Fence), "Create Fence for CommandBuffer Failed");
	D_LOG("--- CommandBuffer: Vulkan Fence created successfully");

	//Create stackingbuffer
	m_pStack = new StackVK(m_pDevice);
	if (!m_pStack->create(MB(2)))
	{
		return false;
	}

	return true;
}

void CommandBufferVK::reset()
{
	//Wait for GPU to finish with this commandbuffer and then reset it
	vkWaitForFences(m_pDevice->getDevice(), 1, &m_Fence, VK_TRUE, UINT64_MAX);
	vkResetFences(m_pDevice->getDevice(), 1, &m_Fence);
}

void CommandBufferVK::begin()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	VK_CHECK_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo), "Begin CommandBuffer Failed");
}

void CommandBufferVK::end()
{
	VK_CHECK_RESULT(vkEndCommandBuffer(m_CommandBuffer), "End CommandBuffer Failed");
}

void CommandBufferVK::beginRenderPass(RenderPassVK* pRenderPass, FrameBufferVK* pFrameBuffer, uint32_t width, uint32_t height, VkClearValue* pClearVales, uint32_t clearValueCount)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.renderPass	= pRenderPass->getRenderPass();
	renderPassInfo.framebuffer	= pFrameBuffer->getFrameBuffer();
	renderPassInfo.renderArea.offset	= { 0, 0 };
	renderPassInfo.renderArea.extent	= { width, height };
	renderPassInfo.pClearValues			= pClearVales;
	renderPassInfo.clearValueCount		= clearValueCount;

	vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBufferVK::endRenderPass()
{
	vkCmdEndRenderPass(m_CommandBuffer);
}

void CommandBufferVK::bindGraphicsPipeline(PipelineVK* pPipeline)
{
	vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->getPipeline());
}

void CommandBufferVK::bindDescriptorSet(VkPipelineBindPoint bindPoint, PipelineLayoutVK* pPipelineLayout, uint32_t firstSet, uint32_t count, const DescriptorSetVK* const* ppDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
	for (uint32_t i = 0; i < count; i++)
	{
		//m_DescriptorSets.emplace_back(ppDescriptorSets[i]->getDescriptorSet());
	}

	//vkCmdBindDescriptorSets(m_CommandBuffer, bindPoint, pPipelineLayout->getPipelineLayout(), firstSet, count, m_DescriptorSet.data(), dynamicOffsetCount, pDynamicOffsets);
	m_DescriptorSets.clear();
}

void CommandBufferVK::setScissorRects(VkRect2D* pScissorRects, uint32_t scissorRectCount)
{
	vkCmdSetScissor(m_CommandBuffer, 0, scissorRectCount, pScissorRects);
}

void CommandBufferVK::setViewports(VkViewport* pViewports, uint32_t viewportCount)
{
	vkCmdSetViewport(m_CommandBuffer, 0, viewportCount, pViewports);
}

void CommandBufferVK::updateBuffer(BufferVK* pDestination, uint64_t destinationOffset, const void* pSource, uint64_t sizeInBytes)
{
	void* pHostMemory = m_pStack->allocate(sizeInBytes);
	memcpy(pHostMemory, pSource, sizeInBytes);

	copyBuffer(m_pStack->getBuffer(), m_pStack->getCurrentOffset(), pDestination, destinationOffset, sizeInBytes);
}

void CommandBufferVK::copyBuffer(BufferVK* pSource, uint64_t sourceOffset, BufferVK* pDestination, uint64_t destinationOffset, uint64_t sizeInBytes)
{
	VkBufferCopy bufferCopy = {};
	bufferCopy.size			= sizeInBytes;
	bufferCopy.srcOffset	= sourceOffset;
	bufferCopy.dstOffset	= destinationOffset;

	vkCmdCopyBuffer(m_CommandBuffer, pSource->getBuffer(), pDestination->getBuffer(), 1, &bufferCopy);
}

void CommandBufferVK::updateImage(const void* pPixelData, ImageVK* pImage, uint32_t width, uint32_t height)
{
	uint32_t sizeInBytes = width * height * 4;
	void* pHostMemory = m_pStack->allocate(sizeInBytes);
	memcpy(pHostMemory, pPixelData, sizeInBytes);

	copyBufferToImage(m_pStack->getBuffer(), pImage, width, height);
}

void CommandBufferVK::copyBufferToImage(BufferVK* pSource, ImageVK* pImage, uint32_t width, uint32_t height)
{
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

	vkCmdCopyBufferToImage(m_CommandBuffer, pSource->getBuffer(), pImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}


void CommandBufferVK::transitionImageLayout(ImageVK* pImage, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = pImage->getImage();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else 
	{
		LOG("Unsupported layout transition");
	}

	vkCmdPipelineBarrier(
		m_CommandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

}

void CommandBufferVK::drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}
