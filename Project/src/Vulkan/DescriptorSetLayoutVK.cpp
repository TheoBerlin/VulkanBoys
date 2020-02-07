#include "DescriptorSetLayoutVK.h"

#include "DeviceVK.h"

#include <iostream>

DescriptorSetLayoutVK::DescriptorSetLayoutVK(DeviceVK* pDevice)
    :m_pDevice(pDevice)
{}

DescriptorSetLayoutVK::~DescriptorSetLayoutVK()
{}

void DescriptorSetLayoutVK::addBindingStorageBuffer(VkShaderStageFlags shaderStageFlags, uint32_t bindingSlot, uint32_t descriptorCount)
{
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
    descriptorSetLayoutBinding.binding = bindingSlot;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorSetLayoutBinding.descriptorCount = descriptorCount;
    descriptorSetLayoutBinding.stageFlags = shaderStageFlags;
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

void DescriptorSetLayoutVK::addBindingSampledImage(VkShaderStageFlags shaderStageFlags, const VkSampler* pImmutableSampler, uint32_t bindingSlot, uint32_t descriptorCount)
{
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
    descriptorSetLayoutBinding.binding = bindingSlot;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorSetLayoutBinding.descriptorCount = descriptorCount;
    descriptorSetLayoutBinding.stageFlags = shaderStageFlags;
    descriptorSetLayoutBinding.pImmutableSamplers = pImmutableSampler;

    m_DescriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);
}

bool DescriptorSetLayoutVK::finalizeLayout()
{
	VkDescriptorSetLayoutCreateInfo uniformLayoutInfo = {};
	uniformLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	uniformLayoutInfo.bindingCount = m_DescriptorSetLayoutBindings.size();
	uniformLayoutInfo.pBindings = m_DescriptorSetLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(m_pDevice->getDevice(), &uniformLayoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
		std::cerr << "Failed to create descriptor set layout" << std::endl;
        return true;
	} else {
		std::cerr << "Created descriptor set layout" << std::endl;
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
