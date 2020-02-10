#pragma once
#include "VulkanCommon.h"
#include "DescriptorCounts.h"

#include <vector>

class DeviceVK;

class DescriptorSetLayoutVK
{
public:
    DescriptorSetLayoutVK(DeviceVK* pDevice);
    ~DescriptorSetLayoutVK();

    void addBindingStorageBuffer(VkShaderStageFlags shaderStageFlags, uint32_t bindingSlot, uint32_t descriptorCount);
    void addBindingUniformBuffer(VkShaderStageFlags shaderStageFlags, uint32_t bindingSlot, uint32_t descriptorCount);
    void addBindingSampledImage(VkShaderStageFlags shaderStageFlags, uint32_t bindingSlot, uint32_t descriptorCount);
    void addBindingCombinedImage(VkShaderStageFlags shaderStageFlags, const VkSampler* pImmutableSampler, uint32_t bindingSlot, uint32_t descriptorCount);
    bool finalize();

    VkDescriptorSetLayout getLayout() const { return m_DescriptorSetLayout; }
    DescriptorCounts getBindingCounts() const;

private:
    DeviceVK* m_pDevice;
    std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBindings;
    VkDescriptorSetLayout m_DescriptorSetLayout;
};
