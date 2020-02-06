#pragma once

#include "vulkan/vulkan.h"

#include <vector>

class DeviceVK;

class DescriptorSetLayoutVK
{
public:
    DescriptorSetLayoutVK(DeviceVK* pDevice);
    ~DescriptorSetLayoutVK();

    void addBindingStorageBuffer(VkShaderStageFlags shaderStageFlags, uint32_t bindingSlot, uint32_t descriptorCount);
    void addBindingUniformBuffer(VkShaderStageFlags shaderStageFlags, uint32_t bindingSlot, uint32_t descriptorCount);
    void addBindingSampledImage(VkShaderStageFlags shaderStageFlags, const VkSampler* pImmutableSampler, uint32_t bindingSlot, uint32_t descriptorCount);
    bool finalizeLayout();

    void createDescriptorSetLayouts(DeviceVK* pDevice, const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBindings);

private:
    DeviceVK* m_pDevice;
    std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBindings;
    VkDescriptorSetLayout m_DescriptorSetLayout;
};
