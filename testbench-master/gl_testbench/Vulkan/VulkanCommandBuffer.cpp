#include "VulkanCommandBuffer.h"

VulkanCommandBuffer::VulkanCommandBuffer()
{}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    this->release();
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

    if (vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
}

void VulkanCommandBuffer::release()
{
    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    vkDestroyFence(m_Device, m_InFlightFence, nullptr);
}
