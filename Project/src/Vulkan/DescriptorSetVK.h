#pragma once
#include "VulkanCommon.h"
#include "DescriptorCounts.h"

class DescriptorPoolVK;
class DeviceVK;

class DescriptorSetVK
{
public:
    DescriptorSetVK();
    ~DescriptorSetVK();

    void initialize(VkDescriptorSet descriptorSetHandle, DeviceVK* pDevice, DescriptorPoolVK* pDescriptorPool, const DescriptorCounts& descriptorCounts);

    VkDescriptorSet getDescriptorSet() const { return m_DescriptorSet; }
    const DescriptorCounts& getDescriptorCounts() const { return m_DescriptorCounts; }

    void writeUniformBufferDescriptor(VkBuffer buffer, uint32_t binding);
    void writeStorageBufferDescriptor(VkBuffer buffer, uint32_t binding);
    void writeCombinedImageDescriptor(VkImageView imageView, VkSampler sampler, uint32_t binding);
    void writeSampledImageDescriptor(VkImageView imageView, uint32_t binding);

private:
    void writeBufferDescriptor(VkBuffer buffer, uint32_t binding, VkDescriptorType bufferType);
    void writeImageDescriptor(VkImageView imageView, VkSampler sampler, uint32_t binding, VkImageLayout layout, VkDescriptorType descriptorType);

private:
    VkDescriptorSet m_DescriptorSet;
    DescriptorCounts m_DescriptorCounts;

    // The pool that allocated the set
    DescriptorPoolVK* m_pDescriptorPool;
    DeviceVK* m_pDevice;
};
