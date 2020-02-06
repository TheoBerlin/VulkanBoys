#pragma once
#include "VulkanCommon.h"

#include <vector>

class StackVK;
class DeviceVK;
class BufferVK;
class RenderPassVK;
class FramebufferVK;
class DescriptorSetVK;
class PipelineStateVK;
class PipelineLayoutVK;

class CommandBufferVK
{
	friend class CommandPoolVK;

public:
	CommandBufferVK(DeviceVK* pDevice);
	~CommandBufferVK();

	DECL_NO_COPY(CommandBufferVK);

	void begin();
	void end();

	void beginRenderPass(RenderPassVK* pRenderPass, FramebufferVK* pFrameBuffer,  uint32_t width, uint32_t height, VkClearValue* pClearVales, uint32_t clearValueCount);
	void endRenderPass();
	
	void bindGraphicsPipelineState(PipelineStateVK* pPipelineState);
	void bindDescriptorSet(VkPipelineBindPoint bindPoint, PipelineLayoutVK* pPipelineLayout, uint32_t firstSet, uint32_t count, const DescriptorSetVK* const * ppDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets);
	
	void setScissorRects(VkRect2D* pScissorRects, uint32_t scissorRectCount);
	void setViewports(VkViewport* pViewports, uint32_t viewportCount);
	
	void updateBuffer(BufferVK* pDestination, uint64_t destinationOffset, const void* pSource, uint64_t sizeInBytes);
	void copyBuffer(BufferVK* pSource, uint64_t sourceOffset, BufferVK* pDestination, uint64_t destinationOffset, uint64_t sizeInBytes);

	void drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);

private:
	bool finalize();

private:
	DeviceVK* m_pDevice;
	StackVK* m_pStack;
	VkFence m_Fence;
	VkCommandBuffer m_CommandBuffer;
	std::vector<VkDescriptorSet> m_DescriptorSets;
};