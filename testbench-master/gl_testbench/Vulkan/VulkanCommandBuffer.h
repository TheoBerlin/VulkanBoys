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

    void reset();
    void beginCommandBuffer();
    void beginRenderPass(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D extent, VkClearValue* pClearVales, uint32_t clearValues);
    void endRenderPass();
	void bindPipelineState(VkPipeline pipelineState);
    void endCommandBuffer();

    VkFence getFence() const { return m_InFlightFence; }
    VkCommandBuffer getCommandBuffer() const { return m_CommandBuffer; }
private:
    VkCommandPool m_CommandPool;
    VkCommandBuffer m_CommandBuffer;

    VkFence m_InFlightFence;

    // Needed for deleting pool
    VkDevice m_Device;
};
