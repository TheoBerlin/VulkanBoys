#pragma once
#include "VulkanCommon.h"
#include "DescriptorSetLayoutVK.h"

class DeviceVK;

class PipelineLayoutVK
{
public:
    PipelineLayoutVK(DeviceVK* pDevice);
    ~PipelineLayoutVK();

    bool init(const std::vector<const DescriptorSetLayoutVK*>& descriptorSetLayouts, const std::vector<VkPushConstantRange>& pushConstantRanges);

    VkPipelineLayout getPipelineLayout() { return m_PipelineLayout; }

private:
    DeviceVK* m_pDevice;
    VkPipelineLayout m_PipelineLayout;
};
