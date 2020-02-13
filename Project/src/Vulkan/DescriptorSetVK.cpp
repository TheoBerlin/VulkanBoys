#include "DescriptorSetVK.h"
#include "DescriptorPoolVK.h"
#include "DeviceVK.h"
#include "BufferVK.h"
#include "SamplerVK.h"
#include "ImageViewVK.h"

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

void DescriptorSetVK::writeUniformBufferDescriptor(BufferVK* pBuffer, uint32_t binding)
{
    writeBufferDescriptor(pBuffer, binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
}

void DescriptorSetVK::writeStorageBufferDescriptor(BufferVK* pBuffer, uint32_t binding)
{
    writeBufferDescriptor(pBuffer, binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
}

void DescriptorSetVK::writeCombinedImageDescriptor(ImageViewVK* pImageView, SamplerVK* pSampler, uint32_t binding)
{
    ASSERT(pSampler != nullptr);
    writeImageDescriptor(pImageView, pSampler, binding, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
}

void DescriptorSetVK::writeSampledImageDescriptor(ImageViewVK* pImageView, uint32_t binding)
{
    writeImageDescriptor(pImageView, nullptr, binding, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
}

void DescriptorSetVK::writeBufferDescriptor(BufferVK* pBuffer, uint32_t binding, VkDescriptorType bufferType)
{
    ASSERT(pBuffer != nullptr);

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer	= pBuffer->getBuffer();
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

void DescriptorSetVK::writeImageDescriptor(ImageViewVK* pImageView, SamplerVK* pSampler, uint32_t binding, VkImageLayout layout, VkDescriptorType descriptorType)
{
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageView     = (pImageView != nullptr) ? pImageView->getImageView() : VK_NULL_HANDLE;
    imageInfo.sampler       = (pSampler != nullptr) ? pSampler->getSampler() : VK_NULL_HANDLE;
    imageInfo.imageLayout   = layout;

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
