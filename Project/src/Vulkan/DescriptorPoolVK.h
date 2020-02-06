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

    void initializeDescriptorPool(DeviceVK* pDevice, uint32_t frameIndex, uint32_t vertexBufferDescriptorCount, uint32_t constantBufferDescriptorCount, uint32_t samplerDescriptorCount, uint32_t descriptorSetCount);

    DescriptorSetVK* allocDescriptorSet(const DescriptorSetLayoutVK* pDescriptorSetLayout);

private:
    DeviceVK* m_pDevice;
    VkDescriptorPool m_DescriptorPools[MAX_FRAMES_IN_FLIGHT];
};
