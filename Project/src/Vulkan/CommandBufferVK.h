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

	void reset();
	void begin();
	void end();

	void beginRenderPass(RenderPassVK* pRenderPass, FrameBufferVK* pFrameBuffer,  uint32_t width, uint32_t height, VkClearValue* pClearVales, uint32_t clearValueCount);
	void endRenderPass();
	
	void bindVertexBuffers(const BufferVK* const * ppVertexBuffers, uint32_t vertexBufferCount, const VkDeviceSize* pOffsets);
	void bindIndexBuffer(const BufferVK* pIndexBuffer, VkDeviceSize offset, VkIndexType indexType);
	void bindGraphicsPipeline(PipelineVK* pPipelineState);
	void bindDescriptorSet(VkPipelineBindPoint bindPoint, PipelineLayoutVK* pPipelineLayout, uint32_t firstSet, uint32_t count, const DescriptorSetVK* const * ppDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets);
	
	void pushConstants(PipelineLayoutVK* pPipelineLayout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues);

	void setScissorRects(VkRect2D* pScissorRects, uint32_t scissorRectCount);
	void setViewports(VkViewport* pViewports, uint32_t viewportCount);
	
	void updateBuffer(BufferVK* pDestination, uint64_t destinationOffset, const void* pSource, uint64_t sizeInBytes);
	void copyBuffer(BufferVK* pSource, uint64_t sourceOffset, BufferVK* pDestination, uint64_t destinationOffset, uint64_t sizeInBytes);

	void updateImage(const void* pPixelData, ImageVK* pImage, uint32_t width, uint32_t height);
	void copyBufferToImage(BufferVK* pSource, VkDeviceSize sourceOffset, ImageVK* pImage, uint32_t width, uint32_t height);
	
	void transitionImageLayout(ImageVK* pImage, VkImageLayout oldLayout, VkImageLayout newLayout);
	
	void drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
	void drawIndexInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);

	//Ray Tracing
	void traceRays(ShaderBindingTableVK* pShaderBindingTable, uint32_t width, uint32_t height, uint32_t raygenOffset);

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