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

	void setPipelineState(VkPipeline pipelineState);

private:
    VkCommandPool m_CommandPool;
    VkCommandBuffer m_CommandBuffer;

    VkFence m_InFlightFence;

    // Needed for deleting pool
    VkDevice m_Device;
};
