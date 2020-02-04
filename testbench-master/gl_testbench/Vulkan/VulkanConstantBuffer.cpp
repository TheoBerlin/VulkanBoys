#include "VulkanConstantBuffer.h"

#include "VulkanRenderer.h"
#include "VulkanDevice.h"

VulkanConstantBuffer::VulkanConstantBuffer(std::string NAME, unsigned int location, unsigned int currentFrame)
    :m_Name(NAME),
    m_Location(location),
    m_CurrentFrame(currentFrame)
{
    for (PerFrameResources& frameResources : m_PerFrameResources) {
        frameResources.m_BufferHandle = VK_NULL_HANDLE;
        frameResources.m_BufferMemory = VK_NULL_HANDLE;
    }
}

VulkanConstantBuffer::~VulkanConstantBuffer()
{
	if (m_pDevice->getDevice() != VK_NULL_HANDLE)
		vkDeviceWaitIdle(m_pDevice->getDevice());

    for (PerFrameResources& frameResources : m_PerFrameResources) {
        if (frameResources.m_BufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_pDevice->getDevice(), frameResources.m_BufferMemory, nullptr);
        }

        if (frameResources.m_BufferHandle != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_pDevice->getDevice(), frameResources.m_BufferHandle, nullptr);
        }
    }
}

void VulkanConstantBuffer::provideResources(VulkanRenderer* pRenderer, VulkanDevice* pVulkanDevice)
{
    m_pRenderer = pRenderer;
	m_pDevice = pVulkanDevice;
}

void VulkanConstantBuffer::initialize(VkDeviceSize size)
{
    for (PerFrameResources& frameResources : m_PerFrameResources) {
        if (frameResources.m_BufferHandle != VK_NULL_HANDLE) {
            continue;
        }

        m_pRenderer->createBuffer(frameResources.m_BufferHandle, frameResources.m_BufferMemory, size, 0, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

        if (frameResources.m_BufferHandle == VK_NULL_HANDLE) {
            throw std::runtime_error("buffer handle is VK_NULL_HANDLE after creation!");
        }
    }
}

void VulkanConstantBuffer::setData(const void* data, size_t size, Material* m, unsigned int location)
{
    // Copy data to staging buffer
    PerFrameResources& frameResources = m_PerFrameResources[m_CurrentFrame];

    if (frameResources.m_BufferHandle == VK_NULL_HANDLE) {
        initialize(size);
    }

    void* dest;
    vkMapMemory(m_pDevice->getDevice(), frameResources.m_BufferMemory, 0, size, 0, &dest);
    memcpy(dest, &data, size);
    vkUnmapMemory(m_pDevice->getDevice(), frameResources.m_BufferMemory);
}

void VulkanConstantBuffer::bind(Material* material)
{
    m_pRenderer->setConstantBuffer(this, m_Location);
}
