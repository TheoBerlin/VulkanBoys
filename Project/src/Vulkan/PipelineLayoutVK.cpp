#include "PipelineLayoutVK.h"

#include "DeviceVK.h"

PipelineLayoutVK::PipelineLayoutVK()
{}

PipelineLayoutVK::~PipelineLayoutVK()
{
    if (m_pDevice != nullptr && m_PipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_pDevice->getDevice(), m_PipelineLayout, nullptr);
    }
}

void PipelineLayoutVK::createPipelineLayout(DeviceVK* pDevice, std::vector<const DescriptorSetLayoutVK*> descriptorSetLayouts)
{
    m_pDevice = pDevice;

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
	pipelineLayoutInfo.pSetLayouts				= vkDescriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount	= 0;
	pipelineLayoutInfo.pPushConstantRanges		= nullptr;

	if (vkCreatePipelineLayout(m_pDevice->getDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
		std::cout << "Failed to create PipelineLayout" << std::endl;
	} else {
		std::cout << "Created PipelineLayout" << std::endl;
	}
}
