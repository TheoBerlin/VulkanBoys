#include "VulkanCommandBuffer.h"

VulkanCommandBuffer::VulkanCommandBuffer()
{}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyFence(device, inFlightFence, nullptr);
}

void VulkanCommandBuffer::initialize(VulkanDevice* device)
{
    this->device = device->getDevice();

    // Create command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = device->getQueueFamilyIndices().graphicsFamily.value();
    poolInfo.flags = 0;

    if (vkCreateCommandPool(this->device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }

    // Create command buffer
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(this->device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    // Create fence
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if (vkCreateFence(this->device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to create synchronization objects for a frame!");
    }
}
