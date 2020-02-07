#pragma once

#include "DescriptorCounts.h"
#include "vulkan/vulkan.h"

#include <list>

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

    DescriptorSetVK* allocDescriptorSet(const DescriptorSetLayoutVK* pDescriptorSetLayout);
    void deallocateDescriptorSet(DescriptorSetVK* pDescriptorSet);

private:
    std::list<DescriptorSetVK*> m_AllocatedSets;
    DescriptorCounts m_DescriptorCounts, m_DescriptorCapacities;

    DeviceVK* m_pDevice;
    VkDescriptorPool m_DescriptorPool;
};
