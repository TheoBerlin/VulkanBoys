#include "DescriptorPoolVK.h"

#include <iostream>

DescriptorPoolVK::DescriptorPoolVK()
    :m_pDevice(nullptr)
{
    for (VkDescriptorPool& descriptorPool : m_DescriptorPools) {
        descriptorPool = VK_NULL_HANDLE;
    }
}

DescriptorPoolVK::~DescriptorPoolVK()
{
    if (m_pDevice == nullptr) {
        return;
    }

    for (VkDescriptorPool& descriptorPool : m_DescriptorPools) {
        if (descriptorPool == VK_NULL_HANDLE) {
            continue;
        }

        vkDestroyDescriptorPool(m_pDevice->getDevice(), descriptorPool, nullptr);
    }
}

void DescriptorPoolVK::initializeDescriptorPool(DeviceVK* pDevice, uint32_t frameIndex, uint32_t vertexBufferDescriptorCount, uint32_t constantBufferDescriptorCount, uint32_t samplerDescriptorCount, uint32_t descriptorSetCount)
{
    m_pDevice = pDevice;

	VkDescriptorPoolSize poolSizes[3];

	poolSizes[0] = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[0].descriptorCount = vertexBufferDescriptorCount;

	poolSizes[1] = {};
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[1].descriptorCount = constantBufferDescriptorCount;

	poolSizes[2] = {};
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = samplerDescriptorCount;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = ARRAYSIZE(poolSizes);
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = descriptorSetCount;

	if (vkCreateDescriptorPool(pDevice->getDevice(), &poolInfo, nullptr, &m_DescriptorPools[frameIndex]) != VK_SUCCESS) {
		std::cerr << "Failed to create Descriptor Pool" << std::endl;
	}
	else {
		/*std::cout << "Created descriptorpool." <<
			"  sets="				<< descriptorSetCount << 
			", storagebuffers="		<< vertexBufferDescriptorCount << 
			", uniformBuffers="		<< constantBufferDescriptorCount << 
			", imageSamplers="		<< samplerDescriptorCount 
			<< std::endl;*/
	}
}

DescriptorSetVK* DescriptorPoolVK::allocDescriptorSet(const DescriptorSetLayoutVK* pDescriptorSetLayout)
{
    
}
