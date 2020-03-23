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
	return	m_DescriptorCounts.m_StorageBuffers			+ allocations.m_StorageBuffers 			< m_DescriptorCapacities.m_StorageBuffers &&
			m_DescriptorCounts.m_UniformBuffers			+ allocations.m_UniformBuffers 			< m_DescriptorCapacities.m_UniformBuffers &&
			m_DescriptorCounts.m_SampledImages 			+ allocations.m_SampledImages			< m_DescriptorCapacities.m_SampledImages  &&
			m_DescriptorCounts.m_StorageImages			+ allocations.m_StorageImages			< m_DescriptorCapacities.m_StorageImages  &&
			m_DescriptorCounts.m_AccelerationStructures + allocations.m_AccelerationStructures	< m_DescriptorCapacities.m_AccelerationStructures;
}

bool DescriptorPoolVK::init(const DescriptorCounts& descriptorCounts, uint32_t descriptorSetCount)
{
	m_DescriptorCapacities = descriptorCounts;

	//size_t descriptorTypeCount = 0;
	//if (descriptorCounts.m_StorageBuffers > 0) {

	//}

	std::array<VkDescriptorPoolSize, 5> poolSizes;
	poolSizes[0].type				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[0].descriptorCount	= descriptorCounts.m_StorageBuffers;

	poolSizes[1].type				= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount	= descriptorCounts.m_UniformBuffers;

	poolSizes[2].type				= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount	= descriptorCounts.m_SampledImages;

	poolSizes[3].type				= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[3].descriptorCount	= descriptorCounts.m_StorageImages;

	poolSizes[4].type				= VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
	poolSizes[4].descriptorCount	= descriptorCounts.m_AccelerationStructures;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount	= uint32_t(descriptorCounts.getDescriptorTypesCount());
	poolInfo.pPoolSizes		= poolSizes.data();
	poolInfo.maxSets		= descriptorSetCount;
	poolInfo.pNext			= nullptr;
	poolInfo.flags			= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateDescriptorPool(m_pDevice->getDevice(), &poolInfo, nullptr, &m_DescriptorPool), "Failed to create Descriptor Pool");
	
	D_LOG("Created descriptorpool. sets=%d, storagebuffers=%d, uniformBuffers=%d, imageSamplers=%d, storageImages=%d, accelerationStructures=%d",
		descriptorSetCount, descriptorCounts.m_StorageBuffers, descriptorCounts.m_UniformBuffers, descriptorCounts.m_SampledImages, descriptorCounts.m_StorageImages, descriptorCounts.m_AccelerationStructures);
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

	DescriptorSetVK* pDescriptorSet = DBG_NEW DescriptorSetVK();
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
