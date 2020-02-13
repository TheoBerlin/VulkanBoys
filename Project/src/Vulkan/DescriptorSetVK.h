#pragma once
#include "VulkanCommon.h"
#include "DescriptorCounts.h"

class DeviceVK;
class BufferVK;
class SamplerVK;
class ImageViewVK;
class DescriptorPoolVK;

class DescriptorSetVK
{
public:
    DescriptorSetVK();
    ~DescriptorSetVK();

    void init(VkDescriptorSet descriptorSetHandle, DeviceVK* pDevice, DescriptorPoolVK* pDescriptorPool, const DescriptorCounts& descriptorCounts);

    void writeUniformBufferDescriptor(BufferVK* pBuffer, uint32_t binding);
    void writeStorageBufferDescriptor(BufferVK* pBuffer, uint32_t binding);
    void writeCombinedImageDescriptor(ImageViewVK* pImageView, SamplerVK* pSampler, uint32_t binding);
    void writeSampledImageDescriptor(ImageViewVK* pImageView, uint32_t binding);
    
    VkDescriptorSet getDescriptorSet() const { return m_DescriptorSet; }
    const DescriptorCounts& getDescriptorCounts() const { return m_DescriptorCounts; }

private:
    void writeBufferDescriptor(BufferVK* pBufferr, uint32_t binding, VkDescriptorType bufferType);
    void writeImageDescriptor(ImageViewVK* pImageView, SamplerVK* pSampler, uint32_t binding, VkImageLayout layout, VkDescriptorType descriptorType);

private:
    DescriptorCounts m_DescriptorCounts;
    VkDescriptorSet m_DescriptorSet;

    // The pool that allocated the set
    DescriptorPoolVK* m_pDescriptorPool;
    DeviceVK* m_pDevice;
};
