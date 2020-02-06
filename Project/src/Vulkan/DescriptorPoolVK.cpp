#include "DescriptorPoolVK.h"

#include "DescriptorSetLayoutVK.h"
#include "DescriptorSetVK.h"

#include <array>
#include <iostream>

DescriptorPoolVK::DescriptorPoolVK()
    :m_pDevice(nullptr),
	m_DescriptorCounts({0}),
	m_DescriptorCapacities({0})
{
    m_DescriptorPool = VK_NULL_HANDLE;
}

DescriptorPoolVK::~DescriptorPoolVK()
{
    if (m_pDevice != nullptr && m_DescriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(m_pDevice->getDevice(), m_DescriptorPool, nullptr);
    }
}

bool DescriptorPoolVK::hasRoomFor(const DescriptorCounts& allocations)
{
	return	m_DescriptorCounts.m_StorageBuffers + allocations.m_StorageBuffers 	< m_DescriptorCapacities.m_StorageBuffers &&
			m_DescriptorCounts.m_UniformBuffers + allocations.m_UniformBuffers 	< m_DescriptorCapacities.m_UniformBuffers &&
			m_DescriptorCounts.m_SampledImages 	+ allocations.m_SampledImages	< m_DescriptorCapacities.m_SampledImages;
}

void DescriptorPoolVK::initializeDescriptorPool(DeviceVK* pDevice, const DescriptorCounts& descriptorCounts, uint32_t descriptorSetCount)
{
    m_pDevice = pDevice;
	m_DescriptorCapacities = descriptorCounts;

	std::array<VkDescriptorPoolSize, 3> poolSizes;

	poolSizes[0] = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[0].descriptorCount = descriptorCounts.m_StorageBuffers;

	poolSizes[1] = {};
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = descriptorCounts.m_UniformBuffers;

	poolSizes[2] = {};
	// TODO: The two things below are not the same
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = descriptorCounts.m_SampledImages;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = descriptorSetCount;

	if (vkCreateDescriptorPool(pDevice->getDevice(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
		std::cerr << "Failed to create Descriptor Pool" << std::endl;
	} else {
		/*std::cout << "Created descriptorpool." <<
			"  sets="				<< descriptorSetCount << 
			", storagebuffers="		<< vertexBufferDescriptorCount << 
			", uniformBuffers="		<< constantBufferDescriptorCount << 
			", imageSamplers="		<< samplerDescriptorCount 
			<< std::endl;*/
	}
}

bool DescriptorPoolVK::allocDescriptorSet(DescriptorSetVK* pDescriptorSet, const DescriptorSetLayoutVK* pDescriptorSetLayout)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = 1;

	VkDescriptorSetLayout layout = pDescriptorSetLayout->getLayout();
	allocInfo.pSetLayouts = &layout;

	if (vkAllocateDescriptorSets(m_pDevice->getDevice(), &allocInfo, pDescriptorSet->getDescriptorSet()) != VK_SUCCESS) {
		std::cout << "Failed to allocate descriptor set" << std::endl;
		return false;
	}

	pDescriptorSet->initialize(m_pDevice, this, pDescriptorSetLayout->getBindingCounts());
	return true;
}

void DescriptorPoolVK::deallocateDescriptorSet(DescriptorSetVK* pDescriptorSet)
{
	if (vkFreeDescriptorSets(m_pDevice->getDevice(), m_DescriptorPool, 1, pDescriptorSet->getDescriptorSet()) != VK_SUCCESS) {
		std::cout << "Failed to deallocate descriptor set" << std::endl;
	}
}
