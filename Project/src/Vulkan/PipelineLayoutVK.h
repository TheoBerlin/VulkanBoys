#pragma once
#include "VulkanCommon.h"
#include "DescriptorSetLayoutVK.h"

class DeviceVK;

class PipelineLayoutVK
{
public:
    PipelineLayoutVK();
    ~PipelineLayoutVK();

    void createPipelineLayout(DeviceVK* pDevice, std::vector<const DescriptorSetLayoutVK*> descriptorSetLayouts);

    VkPipelineLayout getPipelineLayout() { return m_PipelineLayout; }

private:
    DeviceVK* m_pDevice;
    VkPipelineLayout m_PipelineLayout;
};
