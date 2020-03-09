#pragma once
#include "VulkanCommon.h"

#include <vector>

class ImageVK;
class StagingBufferVK;
class DeviceVK;
class BufferVK;
class RenderPassVK;
class FrameBufferVK;
class DescriptorSetVK;
class PipelineVK;
class PipelineLayoutVK;

class ShaderBindingTableVK;

class CommandBufferVK
{
	friend class CommandPoolVK;

public:
	DECL_NO_COPY(CommandBufferVK);

	void reset(bool waitForFence);
	
	void begin(VkCommandBufferInheritanceInfo* pInheritaneInfo, VkCommandBufferUsageFlags flags);
	void end();

	void beginRenderPass(RenderPassVK* pRenderPass, FrameBufferVK* pFrameBuffer,  uint32_t width, uint32_t height, VkClearValue* pClearVales, uint32_t clearValueCount, VkSubpassContents subpassContent);
	void endRenderPass();

	void bindVertexBuffers(const BufferVK* const * ppVertexBuffers, uint32_t vertexBufferCount, const VkDeviceSize* pOffsets);
	void bindIndexBuffer(const BufferVK* pIndexBuffer, VkDeviceSize offset, VkIndexType indexType);
	void bindPipeline(PipelineVK* pPipelineState);
	void bindDescriptorSet(VkPipelineBindPoint bindPoint, PipelineLayoutVK* pPipelineLayout, uint32_t firstSet, uint32_t count, const DescriptorSetVK* const * ppDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets);

	void pushConstants(PipelineLayoutVK* pPipelineLayout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues);

	void setScissorRects(VkRect2D* pScissorRects, uint32_t scissorRectCount);
	void setViewports(VkViewport* pViewports, uint32_t viewportCount);

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

	void drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
	void drawIndexInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);

	void executeSecondary(CommandBufferVK* pSecondary);

	//Ray Tracing
	void traceRays(ShaderBindingTableVK* pShaderBindingTable, uint32_t width, uint32_t height, uint32_t raygenOffset);

	void dispatch(const glm::u32vec3& groupSize);

	//GETTERS
	VkFence getFence() const { return m_Fence; }
	VkCommandBuffer getCommandBuffer() const { return m_CommandBuffer; }

private:
	CommandBufferVK(DeviceVK* pDevice, VkCommandBuffer commandBuffer);
	~CommandBufferVK();

	bool finalize();

private:
	std::vector<VkBuffer> m_VertexBuffers;
	std::vector<VkDescriptorSet> m_DescriptorSets;
	DeviceVK* m_pDevice;
	StagingBufferVK* m_pStagingBuffer;
	StagingBufferVK* m_pStagingTexture;
	VkFence m_Fence;
	VkCommandBuffer m_CommandBuffer;
};
