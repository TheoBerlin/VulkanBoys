#include "VulkanConstantBuffer.h"

#include "VulkanRenderer.h"

VulkanConstantBuffer::VulkanConstantBuffer(std::string NAME, unsigned int location)
    :m_Name(NAME),
    m_BufferHandle(VK_NULL_HANDLE)
{}

VulkanConstantBuffer::~VulkanConstantBuffer()
{
    vkFreeMemory(m_Device, m_BufferMemory, nullptr);
    vkDestroyBuffer(m_Device, m_BufferHandle, nullptr);
}

void VulkanConstantBuffer::provideResources(VulkanRenderer* renderer)
{
    m_pRenderer = renderer;
}

void VulkanConstantBuffer::initialize(VkDeviceSize size)
{
    m_pRenderer->createBuffer(m_BufferHandle, m_BufferMemory, size, 0, 
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

void VulkanConstantBuffer::setData(const void* data, size_t size, Material* m, unsigned int location)
{
    if (m_BufferHandle == VK_NULL_HANDLE) {
        initialize(size);
    }

    void* dest;
    vkMapMemory(m_Device, m_BufferMemory, 0, size, 0, &dest);
    memcpy(dest, &data, size);
    vkUnmapMemory(m_Device, m_BufferMemory);
}

void VulkanConstantBuffer::bind(Material* material)
{
    // Not needed for now, as each buffer has exclusive memory
}
