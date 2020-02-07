#include "CommandBufferVK.h"
#include "DeviceVK.h"
#include "StackVK.h"
#include "BufferVK.h"

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

void CommandBufferVK::begin()
{
	//Wait for GPU to finish with this commandbuffer and then reset it
	vkWaitForFences(m_pDevice->getDevice(), 1, &m_Fence, VK_TRUE, UINT64_MAX);
	vkResetFences(m_pDevice->getDevice(), 1, &m_Fence);

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

void CommandBufferVK::beginRenderPass(RenderPassVK* pRenderPass, FramebufferVK* pFrameBuffer, uint32_t width, uint32_t height, VkClearValue* pClearVales, uint32_t clearValueCount)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.pNext = nullptr;
	//renderPassInfo.renderPass		= pRenderPass->getRenderPass();
	//renderPassInfo.framebuffer	= pFrameBuffer->getFrameBuffer();
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

void CommandBufferVK::bindGraphicsPipelineState(PipelineStateVK* pPipelineState)
{
	//vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineState->getPipeline());
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

void CommandBufferVK::drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}
