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

    void init(VkDescriptorSet descriptorSetHandle, DeviceVK* pDevice, DescriptorPoolVK* pDescriptorPool, const DescriptorCounts& descriptorCounts);

    void writeUniformBufferDescriptor(VkBuffer buffer, uint32_t binding);
    void writeStorageBufferDescriptor(VkBuffer buffer, uint32_t binding);
    void writeCombinedImageDescriptor(VkImageView imageView, VkSampler sampler, uint32_t binding);
	void writeCombinedImageDescriptors(VkImageView imageViews[], VkSampler samplers[], uint32_t count, uint32_t binding);
    void writeSampledImageDescriptor(VkImageView imageView, uint32_t binding);
	void writeStorageImageDescriptor(VkImageView imageView, uint32_t binding);
	void writeAccelerationStructureDescriptor(VkAccelerationStructureNV accelerationStructure, uint32_t binding);
	
    VkDescriptorSet getDescriptorSet() const { return m_DescriptorSet; }
    const DescriptorCounts& getDescriptorCounts() const { return m_DescriptorCounts; }

private:
    void writeBufferDescriptor(VkBuffer buffer, uint32_t binding, VkDescriptorType bufferType);
    void writeImageDescriptors(VkImageView imageView[], VkSampler sampler[], uint32_t count, uint32_t binding, VkImageLayout layout, VkDescriptorType descriptorType);

private:
    DescriptorCounts m_DescriptorCounts;
    VkDescriptorSet m_DescriptorSet;

    // The pool that allocated the set
    DescriptorPoolVK* m_pDescriptorPool;
    DeviceVK* m_pDevice;
};
