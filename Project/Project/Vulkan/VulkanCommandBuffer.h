#pragma once

#include "VulkanDevice.h"

#include <vulkan/vulkan.h>

class VulkanCommandBuffer
{
public:
    VulkanCommandBuffer();
    ~VulkanCommandBuffer();
    void release();

    void initialize(VulkanDevice* device);

    VkCommandBuffer getCommandBuffer();

    void reset();
    void beginCommandBuffer();
    void beginRenderPass(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D extent, VkClearValue* pClearVales, uint32_t clearValues);
	void bindPipelineState(VkPipeline pipelineState);
    void bindDescriptorSet(VkPipelineBindPoint bindPoint, VkPipelineLayout pipelineLayout, uint32_t firstSet, uint32_t count, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets);
    void setScissor(VkRect2D scissorRect);
    void setViewport(VkViewport viewport);
    void drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void endRenderPass();
    void endCommandBuffer();

    VkFence getFence() const { return m_InFlightFence; }
    VkCommandBuffer getCommandBuffer() const { return m_CommandBuffer; }
private:
    VkCommandPool m_CommandPool;
    VkCommandBuffer m_CommandBuffer;

    VkFence m_InFlightFence;

    // Needed for deleting pool
    VulkanDevice* m_pDevice;
};
