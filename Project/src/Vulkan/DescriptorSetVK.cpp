#include "DescriptorSetVK.h"

#include "DescriptorPoolVK.h"
#include "DeviceVK.h"

DescriptorSetVK::DescriptorSetVK()
    :m_pDescriptorPool(nullptr),
    m_DescriptorSet(VK_NULL_HANDLE),
    m_DescriptorCounts({0})
{}

DescriptorSetVK::~DescriptorSetVK()
{
}

void DescriptorSetVK::init(VkDescriptorSet descriptorSetHandle, DeviceVK* pDevice, DescriptorPoolVK* pDescriptorPool, const DescriptorCounts& descriptorCounts)
{
    m_DescriptorSet = descriptorSetHandle;
    m_pDevice = pDevice;
    m_pDescriptorPool = pDescriptorPool;
    m_DescriptorCounts = descriptorCounts;
}

void DescriptorSetVK::writeUniformBufferDescriptor(VkBuffer buffer, uint32_t binding)
{
    writeBufferDescriptor(buffer, binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
}

void DescriptorSetVK::writeStorageBufferDescriptor(VkBuffer buffer, uint32_t binding)
{
    writeBufferDescriptor(buffer, binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
}

void DescriptorSetVK::writeCombinedImageDescriptor(VkImageView imageView, VkSampler sampler, uint32_t binding)
{
    writeImageDescriptors(&imageView, &sampler, 1, binding, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
}

void DescriptorSetVK::writeCombinedImageDescriptors(VkImageView imageViews[], VkSampler samplers[], uint32_t count, uint32_t binding)
{
	writeImageDescriptors(imageViews, samplers, count, binding, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
}

void DescriptorSetVK::writeSampledImageDescriptor(VkImageView imageView, uint32_t binding)
{
    writeImageDescriptors(&imageView, nullptr, 1, binding, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
}

void DescriptorSetVK::writeStorageImageDescriptor(VkImageView imageView, uint32_t binding)
{
	writeImageDescriptors(&imageView, nullptr, 1, binding, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
}

void DescriptorSetVK::writeStorageImageDescriptors(VkImageView imageViews[], uint32_t count, uint32_t binding)
{
	writeImageDescriptors(imageViews, nullptr, count, binding, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
}


void DescriptorSetVK::writeAccelerationStructureDescriptor(VkAccelerationStructureNV accelerationStructure, uint32_t binding)
{
	VkWriteDescriptorSetAccelerationStructureNV descriptorAccelerationStructureInfo = {};
	descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &accelerationStructure;

	VkWriteDescriptorSet accelerationStructureWrite = {};
	accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	// The specialized acceleration structure descriptor has to be chained
	accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
	accelerationStructureWrite.dstSet = m_DescriptorSet;
	accelerationStructureWrite.dstBinding = binding;
	accelerationStructureWrite.descriptorCount = 1;
	accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
	accelerationStructureWrite.pBufferInfo = nullptr;
	accelerationStructureWrite.pImageInfo = nullptr;
	accelerationStructureWrite.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(m_pDevice->getDevice(), 1, &accelerationStructureWrite, 0, nullptr);
}

void DescriptorSetVK::writeBufferDescriptor(VkBuffer buffer, uint32_t binding, VkDescriptorType bufferType)
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer	= buffer;
    bufferInfo.offset	= 0;
    bufferInfo.range	= VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptorBufferWrite = {};
    descriptorBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorBufferWrite.dstSet = m_DescriptorSet;
    descriptorBufferWrite.dstBinding = binding;
    descriptorBufferWrite.dstArrayElement = 0;
    descriptorBufferWrite.descriptorType = bufferType;
    descriptorBufferWrite.descriptorCount = 1;
    descriptorBufferWrite.pBufferInfo = &bufferInfo;
    descriptorBufferWrite.pImageInfo = nullptr;
    descriptorBufferWrite.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_pDevice->getDevice(), 1, &descriptorBufferWrite, 0, nullptr);
}

void DescriptorSetVK::writeImageDescriptors(VkImageView imageViews[], VkSampler samplers[], uint32_t count, uint32_t binding, VkImageLayout layout, VkDescriptorType descriptorType)
{
	std::vector<VkDescriptorImageInfo> imageInfos;
	imageInfos.reserve(count);

	for (uint32_t i = 0; i < count; i++)
	{
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageView = imageViews[i];
		imageInfo.sampler = samplers != nullptr ? samplers[i] : nullptr;
		imageInfo.imageLayout = layout;
		imageInfos.push_back(imageInfo);
	}

	VkWriteDescriptorSet descriptorImageWrite = {};
	descriptorImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorImageWrite.dstSet = m_DescriptorSet;
	descriptorImageWrite.dstBinding = binding;
	descriptorImageWrite.dstArrayElement = 0;
	descriptorImageWrite.descriptorType = descriptorType;
	descriptorImageWrite.descriptorCount = imageInfos.size();
	descriptorImageWrite.pBufferInfo = nullptr;
	descriptorImageWrite.pImageInfo = imageInfos.data();
	descriptorImageWrite.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(m_pDevice->getDevice(), 1, &descriptorImageWrite, 0, nullptr);
	
}
