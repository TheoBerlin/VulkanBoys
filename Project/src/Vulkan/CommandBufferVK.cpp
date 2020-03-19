#include "CommandBufferVK.h"
#include "DeviceVK.h"
#include "InstanceVK.h"

#include "Ray Tracing/ShaderBindingTableVK.h"

CommandBufferVK::CommandBufferVK(DeviceVK* pDevice, InstanceVK* pInstance, VkCommandBuffer commandBuffer)
	: m_pDevice(pDevice),
	m_pInstance(pInstance),
	m_pStagingBuffer(nullptr),
	m_pStagingTexture(nullptr),
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

	SAFEDELETE(m_pStagingBuffer);
	SAFEDELETE(m_pStagingTexture);
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

	//Create staging-buffers
	m_pStagingBuffer = DBG_NEW StagingBufferVK(m_pDevice);
	if (!m_pStagingBuffer->init(MB(16)))
	{
		return false;
	}

	m_pStagingTexture = DBG_NEW StagingBufferVK(m_pDevice);
	if (!m_pStagingTexture->init(MB(32)))
	{
		return false;
	}

	return true;
}

void CommandBufferVK::reset(bool waitForFence)
{
	// Only primary buffers need to wait for their fences
	if (waitForFence) 
	{
		// Wait for GPU to finish with this commandbuffer and then reset it
		vkWaitForFences(m_pDevice->getDevice(), 1, &m_Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_pDevice->getDevice(), 1, &m_Fence);
	}

	vkResetCommandBuffer(m_CommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

	m_pStagingBuffer->reset();
	m_pStagingTexture->reset();
}

void CommandBufferVK::updateBuffer(BufferVK* pDestination, uint64_t destinationOffset, const void* pSource, uint64_t sizeInBytes)
{
	VkDeviceSize offset = m_pStagingBuffer->getCurrentOffset();
	void* pHostMemory	= m_pStagingBuffer->allocate(sizeInBytes);
	memcpy(pHostMemory, pSource, sizeInBytes);

	copyBuffer(m_pStagingBuffer->getBuffer(), offset, pDestination, destinationOffset, sizeInBytes);
}

void CommandBufferVK::copyBuffer(BufferVK* pSource, uint64_t sourceOffset, BufferVK* pDestination, uint64_t destinationOffset, uint64_t sizeInBytes)
{
	VkBufferCopy bufferCopy = {};
	bufferCopy.size			= sizeInBytes;
	bufferCopy.srcOffset	= sourceOffset;
	bufferCopy.dstOffset	= destinationOffset;

	vkCmdCopyBuffer(m_CommandBuffer, pSource->getBuffer(), pDestination->getBuffer(), 1, &bufferCopy);
}

void CommandBufferVK::releaseBufferOwnership(BufferVK* pBuffer, VkAccessFlags srcAccessMask, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
	VkBufferMemoryBarrier bufferMemoryBarrier = {};
	bufferMemoryBarrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemoryBarrier.pNext				= nullptr;
	bufferMemoryBarrier.srcAccessMask		= srcAccessMask;
	bufferMemoryBarrier.dstAccessMask		= 0;
	bufferMemoryBarrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
	bufferMemoryBarrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
	bufferMemoryBarrier.offset				= 0;
	bufferMemoryBarrier.size				= VK_WHOLE_SIZE;
	bufferMemoryBarrier.buffer				= pBuffer->getBuffer();

	vkCmdPipelineBarrier(m_CommandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr);
}

void CommandBufferVK::acquireBufferOwnership(BufferVK* pBuffer, VkAccessFlags dstAccessMask, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
	VkBufferMemoryBarrier bufferMemoryBarrier = {};
	bufferMemoryBarrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemoryBarrier.pNext				= nullptr;
	bufferMemoryBarrier.srcAccessMask		= 0;
	bufferMemoryBarrier.dstAccessMask		= dstAccessMask;
	bufferMemoryBarrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
	bufferMemoryBarrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
	bufferMemoryBarrier.offset				= 0;
	bufferMemoryBarrier.size				= VK_WHOLE_SIZE;
	bufferMemoryBarrier.buffer				= pBuffer->getBuffer();

	vkCmdPipelineBarrier(m_CommandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr);
}

void CommandBufferVK::releaseImageOwnership(ImageVK* pImage, VkAccessFlags srcAccessMask, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageAspectFlags aspectMask)
{
	ImageVK* ppImages[1] = { pImage };
	releaseImagesOwnership(ppImages, 1, srcAccessMask, srcQueueFamilyIndex, dstQueueFamilyIndex, srcStageMask, dstStageMask, aspectMask);
}

void CommandBufferVK::acquireImageOwnership(ImageVK* pImage, VkAccessFlags dstAccessMask, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageAspectFlags aspectMask)
{
	ImageVK* ppImages[1] = { pImage };
	acquireImagesOwnership(ppImages, 1, dstAccessMask, srcQueueFamilyIndex, dstQueueFamilyIndex, srcStageMask, dstStageMask, aspectMask);
}

void CommandBufferVK::releaseImagesOwnership(ImageVK* const* ppImages, uint32_t count, VkAccessFlags srcAccessMask, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageAspectFlags aspectMask)
{
	std::vector<VkImageMemoryBarrier> imageMemoryBarriers;
	imageMemoryBarriers.reserve(count);

	for (uint32_t i = 0; i < count; i++)
	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		imageMemoryBarrier.pNext				= nullptr;
		imageMemoryBarrier.srcAccessMask		= srcAccessMask;
		imageMemoryBarrier.dstAccessMask		= 0;
		imageMemoryBarrier.srcQueueFamilyIndex	= srcQueueFamilyIndex;
		imageMemoryBarrier.dstQueueFamilyIndex	= dstQueueFamilyIndex;
		imageMemoryBarrier.image				= ppImages[i]->getImage();
		imageMemoryBarrier.subresourceRange		= { aspectMask, 0, ppImages[i]->getMiplevelCount(), 0, ppImages[i]->getArrayLayers() };
	}

	vkCmdPipelineBarrier(m_CommandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, uint32_t(imageMemoryBarriers.size()), imageMemoryBarriers.data());
}

void CommandBufferVK::acquireImagesOwnership(ImageVK* const * ppImages, uint32_t count, VkAccessFlags dstAccessMask, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageAspectFlags aspectMask)
{
	std::vector<VkImageMemoryBarrier> imageMemoryBarriers;
	imageMemoryBarriers.reserve(count);

	for (uint32_t i = 0; i < count; i++)
	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		imageMemoryBarrier.pNext				= nullptr;
		imageMemoryBarrier.srcAccessMask		= 0;
		imageMemoryBarrier.dstAccessMask		= dstAccessMask;
		imageMemoryBarrier.srcQueueFamilyIndex	= srcQueueFamilyIndex;
		imageMemoryBarrier.dstQueueFamilyIndex	= dstQueueFamilyIndex;
		imageMemoryBarrier.image				= ppImages[i]->getImage();
		imageMemoryBarrier.subresourceRange		= { aspectMask, 0, ppImages[i]->getMiplevelCount(), 0, ppImages[i]->getArrayLayers() };
	}

	vkCmdPipelineBarrier(m_CommandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, uint32_t(imageMemoryBarriers.size()), imageMemoryBarriers.data());
}


void CommandBufferVK::blitImage2D(ImageVK* pSource, uint32_t sourceMip, VkExtent2D sourceExtent, ImageVK* pDestination, uint32_t destinationMip, VkExtent2D destinationExtent)
{
	VkImageBlit blit = {};
	blit.srcOffsets[0]					= { 0, 0, 0 };
	blit.srcOffsets[1]					= { int32_t(sourceExtent.width), int32_t(sourceExtent.height), int32_t(1) };
	blit.srcSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.mipLevel		= sourceMip;
	blit.srcSubresource.baseArrayLayer	= 0;
	blit.srcSubresource.layerCount		= 1;
	blit.dstOffsets[0]					= { 0, 0, 0 };
	blit.dstOffsets[1]					= { int32_t(destinationExtent.width), int32_t(destinationExtent.height), int32_t(1) };
	blit.dstSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	blit.dstSubresource.mipLevel		= destinationMip;
	blit.dstSubresource.baseArrayLayer	= 0;
	blit.dstSubresource.layerCount		= 1;

	vkCmdBlitImage(m_CommandBuffer, pSource->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pDestination->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
}

void CommandBufferVK::updateImage(const void* pPixelData, ImageVK* pImage, uint32_t width, uint32_t height, uint32_t pixelStride, uint32_t miplevel, uint32_t layer)
{
	uint32_t sizeInBytes = width * height * pixelStride;
	
	VkDeviceSize offset = m_pStagingTexture->getCurrentOffset();
	void* pHostMemory	= m_pStagingTexture->allocate(sizeInBytes);
	memcpy(pHostMemory, pPixelData, sizeInBytes);
	
	copyBufferToImage(m_pStagingTexture->getBuffer(), offset, pImage, width, height, miplevel, layer);
}

void CommandBufferVK::copyBufferToImage(BufferVK* pSource, VkDeviceSize sourceOffset, ImageVK* pImage, uint32_t width, uint32_t height, uint32_t miplevel, uint32_t layer)
{
	VkBufferImageCopy region = {};
	region.bufferImageHeight				= 0;
	region.bufferOffset						= sourceOffset;
	region.bufferRowLength					= 0;
	region.imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.baseArrayLayer	= layer;
	region.imageSubresource.layerCount		= 1;
	region.imageSubresource.mipLevel		= miplevel;
	region.imageExtent.depth				= 1;
	region.imageExtent.height				= height;
	region.imageExtent.width				= width;

	vkCmdCopyBufferToImage(m_CommandBuffer, pSource->getBuffer(), pImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}


void CommandBufferVK::transitionImageLayout(ImageVK* pImage, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMiplevel, uint32_t miplevels, uint32_t baseLayer, uint32_t layerCount, VkImageAspectFlagBits aspectMask)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType							= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
	barrier.oldLayout						= oldLayout;
	barrier.newLayout						= newLayout;
	barrier.subresourceRange.aspectMask		= aspectMask;
	barrier.subresourceRange.baseMipLevel	= baseMiplevel;
	barrier.subresourceRange.levelCount		= miplevels;
	barrier.subresourceRange.baseArrayLayer	= baseLayer;
	barrier.subresourceRange.layerCount		= layerCount;
	barrier.image = pImage->getImage();

	VkPipelineStageFlags sourceStage		= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags destinationStage	= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	/*if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage			= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage	= VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;

		sourceStage			= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage	= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage			= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage	= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage			= VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage	= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage			= VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage	= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage			= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage	= VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage			= VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage	= VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}
	else 
	{
		LOG("Unsupported layout transition");
	}*/

	// Source layouts (old)
	// Source access mask controls actions that have to be finished on the old layout
	// before it will be transitioned to the new layout
	switch (oldLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		// Image layout is undefined (or does not matter)
		// Only valid as initial layout
		// No flags required, listed only for completeness
		barrier.srcAccessMask = 0;
		break;

	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		// Image is preinitialized
		// Only valid as initial layout for linear images, preserves memory contents
		// Make sure host writes have been finished
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image is a color attachment
		// Make sure any writes to the color buffer have been finished
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image is a depth/stencil attachment
		// Make sure any writes to the depth/stencil buffer have been finished
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image is a transfer source 
		// Make sure any reads from the image have been finished
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image is a transfer destination
		// Make sure any writes to the image have been finished
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image is read by a shader
		// Make sure any shader reads from the image have been finished
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_GENERAL:
		barrier.srcAccessMask = 0;
		break;
	default:
		// Other source layouts aren't handled (yet)
		D_LOG("Unsupported layout transition");
		break;
	}

	// Target layouts (new)
	// Destination access mask controls the dependency for the new image layout
	switch (newLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image will be used as a transfer destination
		// Make sure any writes to the image have been finished
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image will be used as a transfer source
		// Make sure any reads from the image have been finished
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image will be used as a color attachment
		// Make sure any writes to the color buffer have been finished
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image layout will be used as a depth/stencil attachment
		// Make sure any writes to depth/stencil buffer have been finished
		barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image will be read in a shader (sampler, input attachment)
		// Make sure any writes to the image have been finished
		if (barrier.srcAccessMask == 0)
		{
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_GENERAL:
		barrier.dstAccessMask = 0;		
		break;
	default:
		// Other source layouts aren't handled (yet)
		D_LOG("Unsupported layout transition");
		break;
	}

	vkCmdPipelineBarrier(m_CommandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void CommandBufferVK::traceRays(ShaderBindingTableVK* pShaderBindingTable, uint32_t width, uint32_t height, uint32_t raygenOffset)
{
	VkBuffer bufferSBT = pShaderBindingTable->getBuffer()->getBuffer();

	VkDeviceSize missOffset			= pShaderBindingTable->getBindingOffsetMissShaderGroup();
	VkDeviceSize intersectOffset	= pShaderBindingTable->getBindingOffsetHitShaderGroup();

	m_pDevice->vkCmdTraceRaysNV(m_CommandBuffer,
		bufferSBT, raygenOffset,
		bufferSBT, missOffset, pShaderBindingTable->getBindingStride(),
		bufferSBT, intersectOffset, pShaderBindingTable->getBindingStride(),
		VK_NULL_HANDLE, 0, 0,
		width, height, 1);
}

void CommandBufferVK::setName(const char* pName)
{
	if (pName)
	{
		if (m_pInstance->vkSetDebugUtilsObjectNameEXT)
		{
			VkDebugUtilsObjectNameInfoEXT info = {};
			info.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			info.pNext			= nullptr;
			info.objectType		= VK_OBJECT_TYPE_COMMAND_BUFFER;
			info.objectHandle	= (uint64_t)m_CommandBuffer;
			info.pObjectName	= pName;
			m_pInstance->vkSetDebugUtilsObjectNameEXT(m_pDevice->getDevice(), &info);
		}
	}
}
