#include "DescriptorPoolVK.h"

#include "DescriptorSetLayoutVK.h"
#include "DescriptorSetVK.h"
#include "DeviceVK.h"

#include <array>
#include <iostream>

DescriptorPoolVK::DescriptorPoolVK(DeviceVK* pDevice)
    : m_pDevice(pDevice),
	m_DescriptorPool(VK_NULL_HANDLE),
	m_DescriptorCounts({0}),
	m_DescriptorCapacities({0})
{
}

DescriptorPoolVK::~DescriptorPoolVK()
{
    if (m_pDevice != nullptr && m_DescriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(m_pDevice->getDevice(), m_DescriptorPool, nullptr);
    }

	for (DescriptorSetVK* pAllocatedSet : m_AllocatedSets) {
		SAFEDELETE(pAllocatedSet);
	}
}

bool DescriptorPoolVK::hasRoomFor(const DescriptorCounts& allocations)
{
	return	m_DescriptorCounts.m_StorageBuffers + allocations.m_StorageBuffers 	< m_DescriptorCapacities.m_StorageBuffers &&
			m_DescriptorCounts.m_UniformBuffers + allocations.m_UniformBuffers 	< m_DescriptorCapacities.m_UniformBuffers &&
			m_DescriptorCounts.m_SampledImages 	+ allocations.m_SampledImages	< m_DescriptorCapacities.m_SampledImages;
}

bool DescriptorPoolVK::init(const DescriptorCounts& descriptorCounts, uint32_t descriptorSetCount)
{
	m_DescriptorCapacities = descriptorCounts;

	std::array<VkDescriptorPoolSize, 3> poolSizes;
	poolSizes[0] = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[0].descriptorCount = descriptorCounts.m_StorageBuffers;

	poolSizes[1] = {};
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = descriptorCounts.m_UniformBuffers;

	poolSizes[2] = {};
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = descriptorCounts.m_SampledImages;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = descriptorSetCount;

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateDescriptorPool(m_pDevice->getDevice(), &poolInfo, nullptr, &m_DescriptorPool), "Failed to create Descriptor Pool");
	
	D_LOG("Created descriptorpool. sets=%d, storagebuffers=%d, uniformBuffers=%d, imageSamplers=%d", descriptorSetCount, descriptorCounts.m_StorageBuffers, descriptorCounts.m_UniformBuffers, descriptorCounts.m_SampledImages);
	return true;
}

DescriptorSetVK* DescriptorPoolVK::allocDescriptorSet(const DescriptorSetLayoutVK* pDescriptorSetLayout)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = 1;

	VkDescriptorSetLayout layout = pDescriptorSetLayout->getLayout();
	allocInfo.pSetLayouts = &layout;

	VkDescriptorSet descriptorSetHandle = VK_NULL_HANDLE;
	if (vkAllocateDescriptorSets(m_pDevice->getDevice(), &allocInfo, &descriptorSetHandle) != VK_SUCCESS) {
		std::cout << "Failed to allocate descriptor set" << std::endl;
		return nullptr;
	}

	DescriptorCounts allocatedDescriptorCount = pDescriptorSetLayout->getBindingCounts();
	m_DescriptorCounts += allocatedDescriptorCount;

	DescriptorSetVK* pDescriptorSet = new DescriptorSetVK();
	pDescriptorSet->init(descriptorSetHandle, m_pDevice, this, allocatedDescriptorCount);
	m_AllocatedSets.push_back(pDescriptorSet);

	return pDescriptorSet;
}

void DescriptorPoolVK::deallocateDescriptorSet(DescriptorSetVK* pDescriptorSet)
{
	VkDescriptorSet descriptorSet = pDescriptorSet->getDescriptorSet();
	if (vkFreeDescriptorSets(m_pDevice->getDevice(), m_DescriptorPool, 1, &descriptorSet) != VK_SUCCESS) {
		std::cout << "Failed to deallocate descriptor set" << std::endl;
	}

	m_DescriptorCounts -= pDescriptorSet->getDescriptorCounts();
	m_AllocatedSets.remove(pDescriptorSet);

	SAFEDELETE(pDescriptorSet);
}
