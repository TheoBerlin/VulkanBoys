#pragma once

#include "Common.h"
#include "vulkan/vulkan.h"

class DescriptorSetVK;
class DescriptorSetLayoutVK;
class DeviceVK;

class DescriptorPoolVK
{
public:
    DescriptorPoolVK();
    ~DescriptorPoolVK();

    bool hasRoomFor(const DescriptorCounts& descriptors);

    void initializeDescriptorPool(DeviceVK* pDevice, const DescriptorCounts& descriptorCounts, uint32_t descriptorSetCount);

    bool allocDescriptorSet(DescriptorSetVK* pDescriptorSet, const DescriptorSetLayoutVK* pDescriptorSetLayout);
    void deallocateDescriptorSet(DescriptorSetVK* pDescriptorSet);

private:
    DescriptorCounts m_DescriptorCounts, m_DescriptorCapacities;

    DeviceVK* m_pDevice;
    VkDescriptorPool m_DescriptorPool;
};
