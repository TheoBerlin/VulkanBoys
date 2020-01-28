#pragma once

#include "VulkanDevice.h"

#include <vulkan/vulkan.h>

class VulkanCommandBuffer
{
public:
    VulkanCommandBuffer();
    ~VulkanCommandBuffer();

    void initialize(VulkanDevice* device);

    void release();

private:
    VkCommandPool m_commandPool;
    VkCommandBuffer m_commandBuffer;

    VkFence m_inFlightFence;

    // Needed for deleting pool
    VkDevice m_device;
};
