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

void DescriptorSetVK::initialize(VkDescriptorSet descriptorSetHandle, DeviceVK* pDevice, DescriptorPoolVK* pDescriptorPool, const DescriptorCounts& descriptorCounts)
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
    writeImageDescriptor(imageView, sampler, binding, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
}

void DescriptorSetVK::writeSampledImageDescriptor(VkImageView imageView, uint32_t binding)
{
    writeImageDescriptor(imageView, VK_NULL_HANDLE, binding, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
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

void DescriptorSetVK::writeImageDescriptor(VkImageView imageView, VkSampler sampler, uint32_t binding, VkImageLayout layout, VkDescriptorType descriptorType)
{
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;
    imageInfo.imageLayout = layout;

    VkWriteDescriptorSet descriptorImageWrite = {};
    descriptorImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorImageWrite.dstSet = m_DescriptorSet;
    descriptorImageWrite.dstBinding = binding;
    descriptorImageWrite.dstArrayElement = 0;
    descriptorImageWrite.descriptorType = descriptorType;
    descriptorImageWrite.descriptorCount = 1;
    descriptorImageWrite.pBufferInfo = nullptr;
    descriptorImageWrite.pImageInfo = &imageInfo;
    descriptorImageWrite.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_pDevice->getDevice(), 1, &descriptorImageWrite, 0, nullptr);
}
