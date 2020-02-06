#pragma once

#include "Common.h"
#include "vulkan/vulkan.h"

#define UNIFORM_BUFFERS_PER_POOL 10
#define STORAGE_BUFFERS_PER_POOL 10
#define SAMPLED_IMAGES_PER_POOL 10
#define DESCRIPTOR_SETS_PER_POOL 10

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
