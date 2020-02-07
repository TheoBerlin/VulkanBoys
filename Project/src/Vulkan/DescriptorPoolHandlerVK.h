#pragma once

#include "vulkan/vulkan.h"

#include "DescriptorPoolVK.h"
#include "VulkanCommon.h"

#include <array>
#include <vector>

class DescriptorSetVK;
class DescriptorSetLayoutVK;
class DeviceVK;

class DescriptorPoolHandlerVK
{
public:
    DescriptorPoolHandlerVK();
    ~DescriptorPoolHandlerVK();

    void setCurrentFrame(unsigned int currentFrame) { m_CurrentFrame = currentFrame; }

    // Picks or creates a descriptor pool and allocates the descriptor set
    DescriptorSetVK* allocDescriptorSet(const DescriptorSetLayoutVK* pDescriptorSetLayout);

private:
    DeviceVK* m_pDevice;
    // Each frame has an expandable array of descriptor pools
    std::array<std::vector<DescriptorPoolVK>, MAX_FRAMES_IN_FLIGHT> m_DescriptorPools;

    unsigned int m_CurrentFrame;
};
