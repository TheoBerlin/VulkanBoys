#pragma once
#include <vector>

#include "VulkanCommon.h"
#include "RenderPassVK.h"
#include "FrameBufferVK.h"
#include "PipelineVK.h"
#include "PipelineLayoutVK.h"
#include "ImageVK.h"
#include "BufferVK.h"
#include "StagingBufferVK.h"
#include "DescriptorSetVK.h"

class DeviceVK;
class InstanceVK;
class ShaderBindingTableVK;

class CommandBufferVK
{
	friend class CommandPoolVK;

public:
	DECL_NO_COPY(CommandBufferVK);

	void reset(bool waitForFence);

	void updateBuffer(BufferVK* pDestination, uint64_t destinationOffset, const void* pSource, uint64_t sizeInBytes);
	void copyBuffer(BufferVK* pSource, uint64_t sourceOffset, BufferVK* pDestination, uint64_t destinationOffset, uint64_t sizeInBytes);

	void blitImage2D(ImageVK* pSource, uint32_t sourceMip, VkExtent2D sourceExtent, ImageVK* pDestination, uint32_t destinationMip, VkExtent2D destinationExtent);

	void updateImage(const void* pPixelData, ImageVK* pImage, uint32_t width, uint32_t height, uint32_t pixelStride, uint32_t miplevel, uint32_t layer);
	void copyBufferToImage(BufferVK* pSource, VkDeviceSize sourceOffset, ImageVK* pImage, uint32_t width, uint32_t height, uint32_t miplevel, uint32_t layer);

	void releaseBufferOwnership(BufferVK* pBuffer, VkAccessFlags srcAccessMask, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);
	void acquireBufferOwnership(BufferVK* pBuffer, VkAccessFlags dstAccessMask, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask);

	void transitionImageLayout(ImageVK* pImage, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMiplevel, uint32_t miplevels, uint32_t baseLayer, uint32_t layerCount, VkImageAspectFlagBits aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

	void releaseImageOwnership(ImageVK* pImage, VkAccessFlags srcAccessMask, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);
	void acquireImageOwnership(ImageVK* pImage, VkAccessFlags dstAccessMask, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);
	void releaseImagesOwnership(ImageVK* const* ppImages, uint32_t count, VkAccessFlags srcAccessMask, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);
	void acquireImagesOwnership(ImageVK* const* ppImages, uint32_t count, VkAccessFlags dstAccessMask, uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

	void traceRays(ShaderBindingTableVK* pShaderBindingTable, uint32_t width, uint32_t height, uint32_t raygenOffset);

	void setName(const char* pName);

	FORCEINLINE void pipelineBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkDependencyFlags dependencyFlags,
		uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
		uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
	{
		vkCmdPipelineBarrier(m_CommandBuffer, srcStage, dstStage, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
	}

	FORCEINLINE void imageMemoryBarrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
	{
		vkCmdPipelineBarrier(m_CommandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, imageMemoryBarrierCount, pImageMemoryBarriers);
	}

	FORCEINLINE void begin(VkCommandBufferInheritanceInfo* pInheritaneInfo, VkCommandBufferUsageFlags flags)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext				= nullptr;
		beginInfo.flags				= flags;
		beginInfo.pInheritanceInfo	= pInheritaneInfo;

		VK_CHECK_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo), "Begin CommandBuffer Failed");
	}

	FORCEINLINE void bindVertexBuffers(const BufferVK* const* ppVertexBuffers, uint32_t vertexBufferCount, const VkDeviceSize* pOffsets)
	{
		for (uint32_t i = 0; i < vertexBufferCount; i++)
		{
			m_VertexBuffers.emplace_back(ppVertexBuffers[i]->getBuffer());
		}

		vkCmdBindVertexBuffers(m_CommandBuffer, 0, vertexBufferCount, m_VertexBuffers.data(), pOffsets);
		m_VertexBuffers.clear();
	}

	FORCEINLINE void bindIndexBuffer(const BufferVK* pIndexBuffer, VkDeviceSize offset, VkIndexType indexType)
	{
		vkCmdBindIndexBuffer(m_CommandBuffer, pIndexBuffer->getBuffer(), offset, indexType);
	}

	FORCEINLINE void bindPipeline(PipelineVK* pPipeline)
	{
		vkCmdBindPipeline(m_CommandBuffer, pPipeline->getBindPoint(), pPipeline->getPipeline());
	}

	FORCEINLINE void bindDescriptorSet(VkPipelineBindPoint bindPoint, PipelineLayoutVK* pPipelineLayout, uint32_t firstSet, uint32_t count, const DescriptorSetVK* const* ppDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
	{
		for (uint32_t i = 0; i < count; i++)
		{
			m_DescriptorSets.emplace_back(ppDescriptorSets[i]->getDescriptorSet());
		}

		vkCmdBindDescriptorSets(m_CommandBuffer, bindPoint, pPipelineLayout->getPipelineLayout(), firstSet, count, m_DescriptorSets.data(), dynamicOffsetCount, pDynamicOffsets);
		m_DescriptorSets.clear();
	}

	FORCEINLINE void beginRenderPass(RenderPassVK* pRenderPass, FrameBufferVK* pFrameBuffer, uint32_t width, uint32_t height, VkClearValue* pClearVales, uint32_t clearValueCount, VkSubpassContents subpassContent)
	{
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.pNext				= nullptr;
		renderPassInfo.renderPass			= pRenderPass->getRenderPass();
		renderPassInfo.framebuffer			= pFrameBuffer->getFrameBuffer();
		renderPassInfo.renderArea.offset	= { 0, 0 };
		renderPassInfo.renderArea.extent	= { width, height };
		renderPassInfo.pClearValues			= pClearVales;
		renderPassInfo.clearValueCount		= clearValueCount;

		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, subpassContent);
	}

	FORCEINLINE void endRenderPass()
	{
		vkCmdEndRenderPass(m_CommandBuffer);
	}

	FORCEINLINE void end()
	{
		VK_CHECK_RESULT(vkEndCommandBuffer(m_CommandBuffer), "End CommandBuffer Failed");
	}

	FORCEINLINE void pushConstants(PipelineLayoutVK* pPipelineLayout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues)
	{
		vkCmdPushConstants(m_CommandBuffer, pPipelineLayout->getPipelineLayout(), stageFlags, offset, size, pValues);
	}

	FORCEINLINE void setScissorRects(VkRect2D* pScissorRects, uint32_t scissorRectCount)
	{
		vkCmdSetScissor(m_CommandBuffer, 0, scissorRectCount, pScissorRects);
	}

	FORCEINLINE void setViewports(VkViewport* pViewports, uint32_t viewportCount)
	{
		vkCmdSetViewport(m_CommandBuffer, 0, viewportCount, pViewports);
	}

	FORCEINLINE void drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	FORCEINLINE void drawIndexInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
	{
		vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	FORCEINLINE void executeSecondary(CommandBufferVK* pSecondary)
	{
		VkCommandBuffer secondaryBuffer = pSecondary->getCommandBuffer();
		vkCmdExecuteCommands(m_CommandBuffer, 1, &secondaryBuffer);
	}

	FORCEINLINE void CommandBufferVK::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		vkCmdDispatch(m_CommandBuffer, groupCountX, groupCountY, groupCountZ);
	}

	FORCEINLINE void CommandBufferVK::dispatch(const glm::u32vec3& groupSize)
	{
		vkCmdDispatch(m_CommandBuffer, groupSize.x, groupSize.y, groupSize.z);
	}

	FORCEINLINE VkFence			getFence() const			{ return m_Fence; }
	FORCEINLINE VkCommandBuffer getCommandBuffer() const	{ return m_CommandBuffer; }

private:
	CommandBufferVK(DeviceVK* pDevice, InstanceVK* pInstance, VkCommandBuffer commandBuffer);
	~CommandBufferVK();

	bool finalize();

private:
	std::vector<VkBuffer> m_VertexBuffers;
	std::vector<VkDescriptorSet> m_DescriptorSets;
	DeviceVK* m_pDevice;
	InstanceVK* m_pInstance;
	StagingBufferVK* m_pStagingBuffer;
	StagingBufferVK* m_pStagingTexture;
	VkFence m_Fence;
	VkCommandBuffer m_CommandBuffer;
};
