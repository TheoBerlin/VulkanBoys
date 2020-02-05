#include "CommandBufferVK.h"
#include "DeviceVK.h"

CommandBufferVK::CommandBufferVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_Fence(VK_NULL_HANDLE),
	m_CommandBuffer(VK_NULL_HANDLE),
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
}

bool CommandBufferVK::finalize()
{
	// Create fence
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateFence(m_pDevice->getDevice(), &fenceInfo, nullptr, &m_Fence), "Create Fence for CommandBuffer Failed");

	//TODO: Create staging buffer

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
	//renderPassInfo.renderPass = pRenderPass->getRenderPass();
	//renderPassInfo.framebuffer = pFrameBuffer->getFrameBuffer();
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

void CommandBufferVK::drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}
