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

    VkDescriptorSet* getDescriptorSet() { return &m_DescriptorSet; }
    const DescriptorCounts& getDescriptorCounts() const { return m_DescriptorCounts; }

    void writeUniformBufferDescriptor(VkBuffer buffer, uint32_t binding);
    void writeStorageBufferDescriptor(VkBuffer buffer, uint32_t binding);
    void writeSampledImageDescriptor(VkImageView imageView, VkSampler sampler, uint32_t binding);

private:
    void writeBufferDescriptor(VkBuffer buffer, uint32_t binding, VkDescriptorType bufferType);

private:
    VkDescriptorSet m_DescriptorSet;
    DescriptorCounts m_DescriptorCounts;

    // The pool that allocated the set
    DescriptorPoolVK* m_pDescriptorPool;
    DeviceVK* m_pDevice;
};
