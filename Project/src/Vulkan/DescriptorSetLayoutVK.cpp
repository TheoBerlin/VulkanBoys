#include "DescriptorSetLayoutVK.h"

#include "DeviceVK.h"

#include <iostream>

DescriptorSetLayoutVK::DescriptorSetLayoutVK(DeviceVK* pDevice)
    : m_pDevice(pDevice),
    m_DescriptorSetLayout(VK_NULL_HANDLE)
{
}

DescriptorSetLayoutVK::~DescriptorSetLayoutVK()
{
    if (m_DescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_pDevice->getDevice(), m_DescriptorSetLayout, nullptr);
        m_DescriptorSetLayout = VK_NULL_HANDLE;
    }
}

void DescriptorSetLayoutVK::addBindingStorageBuffer(VkShaderStageFlags shaderStageFlags, uint32_t bindingSlot, uint32_t descriptorCount)
{
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
    descriptorSetLayoutBinding.binding          = bindingSlot;
    descriptorSetLayoutBinding.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorSetLayoutBinding.descriptorCount  = descriptorCount;
    descriptorSetLayoutBinding.stageFlags       = shaderStageFlags;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    m_DescriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);
}

void DescriptorSetLayoutVK::addBindingUniformBuffer(VkShaderStageFlags shaderStageFlags, uint32_t bindingSlot, uint32_t descriptorCount)
{
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
    descriptorSetLayoutBinding.binding = bindingSlot;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorSetLayoutBinding.descriptorCount = descriptorCount;
    descriptorSetLayoutBinding.stageFlags = shaderStageFlags;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    m_DescriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);
}

void DescriptorSetLayoutVK::addBindingSampledImage(VkShaderStageFlags shaderStageFlags, uint32_t bindingSlot, uint32_t descriptorCount)
{
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
    descriptorSetLayoutBinding.binding = bindingSlot;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorSetLayoutBinding.descriptorCount = descriptorCount;
    descriptorSetLayoutBinding.stageFlags = shaderStageFlags;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    m_DescriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);
}

void DescriptorSetLayoutVK::addBindingCombinedImage(VkShaderStageFlags shaderStageFlags, const VkSampler* pImmutableSampler, uint32_t bindingSlot, uint32_t descriptorCount)
{
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
    descriptorSetLayoutBinding.binding          = bindingSlot;
    descriptorSetLayoutBinding.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetLayoutBinding.descriptorCount  = descriptorCount;
    descriptorSetLayoutBinding.stageFlags       = shaderStageFlags;
    descriptorSetLayoutBinding.pImmutableSamplers = pImmutableSampler;

    m_DescriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);
}

bool DescriptorSetLayoutVK::finalize()
{
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
    descriptorSetLayoutInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.flags         = 0;
    descriptorSetLayoutInfo.pNext         = nullptr;
    descriptorSetLayoutInfo.bindingCount  = m_DescriptorSetLayoutBindings.size();
    descriptorSetLayoutInfo.pBindings     = m_DescriptorSetLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(m_pDevice->getDevice(), &descriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
		LOG("Failed to create descriptor set layout");
        return true;
	} else {
		D_LOG("--- DescriptorSetLayout: Vulkan DescriptorSetLayout created successfully");
        return false;
	}
}

DescriptorCounts DescriptorSetLayoutVK::getBindingCounts() const
{
    DescriptorCounts bindingCounts = {};

    for (const VkDescriptorSetLayoutBinding& binding: m_DescriptorSetLayoutBindings) {
        switch (binding.descriptorType) {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                bindingCounts.m_UniformBuffers += binding.descriptorCount;
                break;
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                bindingCounts.m_StorageBuffers += binding.descriptorCount;
                break;
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                bindingCounts.m_SampledImages += binding.descriptorCount;
                break;
            default:
                break;
        }
    }

    return bindingCounts;
}
