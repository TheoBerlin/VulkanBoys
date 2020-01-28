#pragma once

#include "../VulkanDevice.h"

#include <vulkan/vulkan.h>

class VulkanCommandBuffer
{
public:
    VulkanCommandBuffer();
    ~VulkanCommandBuffer();

    void initialize(VulkanDevice* device);

private:
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    // Is equal to frameBufferCount - 1
    uint32_t maxFramesInFlight;

    VkFence inFlightFence;

    // Needed for deleting pool
    VkDevice device;
};
