#include "VulkanCommandBuffer.h"

VulkanCommandBuffer::VulkanCommandBuffer()
{}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    this->release();
}

void VulkanCommandBuffer::release()
{
	if (m_CommandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		m_CommandPool = VK_NULL_HANDLE;
	}

	if (m_InFlightFence != VK_NULL_HANDLE) {
		vkDestroyFence(m_Device, m_InFlightFence, nullptr);
		m_InFlightFence = VK_NULL_HANDLE;
	}
}

void VulkanCommandBuffer::initialize(VulkanDevice* device)
{
    m_Device = device->getDevice();

    // Create command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = device->getQueueFamilyIndices().graphicsFamily.value();
    poolInfo.flags = 0;

    if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }

    // Create command buffer
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    // Create fence
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFence) != VK_SUCCESS) {
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
    vkWaitForFences(m_Device, 1, &m_InFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_Device, 1, &m_InFlightFence);

    //Avoid using the VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT since we can reuse the memory
    vkResetCommandPool(m_Device, m_CommandPool, 0);
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

void VulkanCommandBuffer::endRenderPass()
{
    vkCmdEndRenderPass(m_CommandBuffer);
}

void VulkanCommandBuffer::bindPipelineState(VkPipeline pipelineState)
{
	vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineState);
}

void VulkanCommandBuffer::endCommandBuffer()
{
    VkResult result = vkEndCommandBuffer(m_CommandBuffer);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkEndCommandBuffer failed. Error: " << result << std::endl;
    }
}
