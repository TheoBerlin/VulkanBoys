#pragma once

#include "Common/IDescriptorSet.h"
#include "VulkanCommon.h"
#include "DescriptorCounts.h"

class DeviceVK;
class BufferVK;
class SamplerVK;
class ImageViewVK;
class DescriptorPoolVK;

class DescriptorSetVK : public IDescriptorSet
{
public:
    DescriptorSetVK();
    ~DescriptorSetVK();

    void init(VkDescriptorSet descriptorSetHandle, DeviceVK* pDevice, DescriptorPoolVK* pDescriptorPool, const DescriptorCounts& descriptorCounts);

    void writeUniformBufferDescriptor(const BufferVK* pBuffer, uint32_t binding);
    void writeStorageBufferDescriptor(const BufferVK* pBuffer, uint32_t binding);
	void writeCombinedImageDescriptors(const ImageViewVK* const * ppImageViews, const SamplerVK* const * ppSamplers, uint32_t count, uint32_t binding);
    void writeSampledImageDescriptor(const ImageViewVK* pImageView, uint32_t binding);
	void writeStorageImageDescriptor(const ImageViewVK* pImageView, uint32_t binding);
	void writeAccelerationStructureDescriptor(VkAccelerationStructureNV accelerationStructure, uint32_t binding);
	
    VkDescriptorSet getDescriptorSet() const { return m_DescriptorSet; }
    const DescriptorCounts& getDescriptorCounts() const { return m_DescriptorCounts; }

private:
    void writeBufferDescriptor(const BufferVK* pBuffer, uint32_t binding, VkDescriptorType descriptorType);
    void writeImageDescriptors(const ImageViewVK* const * ppImageViews, const SamplerVK* const * ppSamplers, uint32_t count, uint32_t binding, VkImageLayout layout, VkDescriptorType descriptorType);

private:
    DescriptorCounts m_DescriptorCounts;
    VkDescriptorSet m_DescriptorSet;

    // The pool that allocated the set
    DescriptorPoolVK* m_pDescriptorPool;
    DeviceVK* m_pDevice;
};
