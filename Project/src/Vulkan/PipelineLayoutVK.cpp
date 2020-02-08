#include "PipelineLayoutVK.h"

#include "DeviceVK.h"

PipelineLayoutVK::PipelineLayoutVK(DeviceVK* pDevice)
	: m_pDevice(pDevice)
{
}

PipelineLayoutVK::~PipelineLayoutVK()
{
    if (m_pDevice != nullptr && m_PipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_pDevice->getDevice(), m_PipelineLayout, nullptr);
    }
}

void PipelineLayoutVK::createPipelineLayout(const std::vector<const DescriptorSetLayoutVK*>& descriptorSetLayouts, const std::vector<VkPushConstantRange>& pushConstantRanges)
{
    // Serialize the vulkan handles for the descriptor set layouts
    std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;
    vkDescriptorSetLayouts.reserve(descriptorSetLayouts.size());
    for (const DescriptorSetLayoutVK* layout : descriptorSetLayouts) {
        vkDescriptorSetLayouts.push_back(layout->getLayout());
    }

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pNext = nullptr;
	pipelineLayoutInfo.flags = 0;
    pipelineLayoutInfo.setLayoutCount			= uint32_t(vkDescriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts				= (vkDescriptorSetLayouts.size() > 0) ? vkDescriptorSetLayouts.data() : nullptr;
	pipelineLayoutInfo.pushConstantRangeCount	= uint32_t(pushConstantRanges.size());
	pipelineLayoutInfo.pPushConstantRanges		= (pushConstantRanges.size() > 0) ? pushConstantRanges.data() : nullptr;

	if (vkCreatePipelineLayout(m_pDevice->getDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) 
	{
		LOG("Failed to create PipelineLayout");
	} else {
		LOG("--- PipelineLayout: Vulkan PipelineLayout created successfully");
	}
}
