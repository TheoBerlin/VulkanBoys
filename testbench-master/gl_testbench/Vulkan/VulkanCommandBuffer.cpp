#include "VulkanCommandBuffer.h"

VulkanCommandBuffer::VulkanCommandBuffer()
{}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    this->release();
}

void VulkanCommandBuffer::release()
{
	if (m_pDevice->getDevice() != VK_NULL_HANDLE)
		vkDeviceWaitIdle(m_pDevice->getDevice());
	
	if (m_CommandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(m_pDevice->getDevice(), m_CommandPool, nullptr);
		m_CommandPool = VK_NULL_HANDLE;
	}

	if (m_InFlightFence != VK_NULL_HANDLE) {
		vkDestroyFence(m_pDevice->getDevice(), m_InFlightFence, nullptr);
		m_InFlightFence = VK_NULL_HANDLE;
	}
}

void VulkanCommandBuffer::initialize(VulkanDevice* device)
{
	m_pDevice = device;

    // Create command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = device->getQueueFamilyIndices().graphicsFamily.value();
    poolInfo.flags = 0;

    if (vkCreateCommandPool(m_pDevice->getDevice(), &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }

    // Create command buffer
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(m_pDevice->getDevice(), &allocInfo, &m_CommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    // Create fence
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(m_pDevice->getDevice(), &fenceInfo, nullptr, &m_InFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
}

VkCommandBuffer VulkanCommandBuffer::getCommandBuffer()
{
    return m_CommandBuffer;
}

void VulkanCommandBuffer::reset()
{
    //Wait for GPU to finish with this commandbuffer and then reset it
    vkWaitForFences(m_pDevice->getDevice(), 1, &m_InFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_pDevice->getDevice(), 1, &m_InFlightFence);

    //Avoid using the VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT since we can reuse the memory
    vkResetCommandPool(m_pDevice->getDevice(), m_CommandPool, 0);
}

void VulkanCommandBuffer::beginCommandBuffer()
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    VkResult result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkBeginCommandBuffer failed. Error: " << result << std::endl;
    }
}

void VulkanCommandBuffer::beginRenderPass(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D extent, VkClearValue* pClearVales, uint32_t clearValues)
{
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = frameBuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent;
    renderPassInfo.pClearValues = pClearVales;
    renderPassInfo.clearValueCount = clearValues;

    vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::setScissor(VkRect2D scissorRect)
{
    vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissorRect);
}

void VulkanCommandBuffer::setViewport(VkViewport viewport)
{
    vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
}

void VulkanCommandBuffer::endRenderPass()
{
    vkCmdEndRenderPass(m_CommandBuffer);
}

void VulkanCommandBuffer::bindPipelineState(VkPipeline pipelineState)
{
	vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineState);
}

void VulkanCommandBuffer::bindDescriptorSet(VkPipelineBindPoint bindPoint, VkPipelineLayout pipelineLayout, uint32_t firstSet, uint32_t count, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    vkCmdBindDescriptorSets(m_CommandBuffer, bindPoint, pipelineLayout, firstSet, count, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

void VulkanCommandBuffer::drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::endCommandBuffer()
{
    VkResult result = vkEndCommandBuffer(m_CommandBuffer);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkEndCommandBuffer failed. Error: " << result << std::endl;
    }
}
