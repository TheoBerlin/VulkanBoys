#include "VulkanConstantBuffer.h"

#include "VulkanCommandBuffer.h"
#include "VulkanRenderer.h"
#include "VulkanDevice.h"

VulkanConstantBuffer::VulkanConstantBuffer(std::string NAME, unsigned int location, unsigned int currentFrame)
    :m_Name(NAME),
    m_Location(location),
	m_Size(0),
    m_CurrentFrame(currentFrame),
	m_BufferHandle(VK_NULL_HANDLE),
	m_BufferMemory(VK_NULL_HANDLE),
	m_IsDirty(false)
{
    for (PerFrameStagingBuffers& stagingBuffer : m_PerFrameStagingBuffers) {
        stagingBuffer.m_BufferHandle = VK_NULL_HANDLE;
        stagingBuffer.m_BufferMemory = VK_NULL_HANDLE;
    }
}

VulkanConstantBuffer::~VulkanConstantBuffer()
{
	if (m_pDevice->getDevice() != VK_NULL_HANDLE)
		vkDeviceWaitIdle(m_pDevice->getDevice());

	if (m_BufferMemory != VK_NULL_HANDLE) 
	{
		vkFreeMemory(m_pDevice->getDevice(), m_BufferMemory, nullptr);
	}

	if (m_BufferHandle != VK_NULL_HANDLE) 
	{
		vkDestroyBuffer(m_pDevice->getDevice(), m_BufferHandle, nullptr);
	}
	
    for (PerFrameStagingBuffers& stagingBuffer : m_PerFrameStagingBuffers) {
        if (stagingBuffer.m_BufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_pDevice->getDevice(), stagingBuffer.m_BufferMemory, nullptr);
        }

        if (stagingBuffer.m_BufferHandle != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_pDevice->getDevice(), stagingBuffer.m_BufferHandle, nullptr);
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
	m_Size = size;
	
	m_pRenderer->createBuffer(m_BufferHandle, m_BufferMemory, size, 0,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	if (m_BufferHandle == VK_NULL_HANDLE) {
		throw std::runtime_error("buffer handle is VK_NULL_HANDLE after creation!");
	}
	
    for (PerFrameStagingBuffers& stagingBuffer : m_PerFrameStagingBuffers) {
        if (stagingBuffer.m_BufferHandle != VK_NULL_HANDLE) {
            continue;
        }

        m_pRenderer->createBuffer(stagingBuffer.m_BufferHandle, stagingBuffer.m_BufferMemory, size, 0, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        if (stagingBuffer.m_BufferHandle == VK_NULL_HANDLE) {
            throw std::runtime_error("buffer handle is VK_NULL_HANDLE after creation!");
        }
    }
}

void VulkanConstantBuffer::setData(const void* data, size_t size, Material* m, unsigned int location)
{
    // Copy data to staging buffer
    PerFrameStagingBuffers& stagingBuffer = m_PerFrameStagingBuffers[m_CurrentFrame];
	
    if (stagingBuffer.m_BufferHandle == VK_NULL_HANDLE) {
        initialize(size);
    }

    void* dest;
    vkMapMemory(m_pDevice->getDevice(), stagingBuffer.m_BufferMemory, 0, size, 0, &dest);
    memcpy(dest, data, size);
    vkUnmapMemory(m_pDevice->getDevice(), stagingBuffer.m_BufferMemory);

	m_IsDirty = true;
}

void VulkanConstantBuffer::copyToBuffer(VulkanCommandBuffer* pCommandBuffer)
{
	if (m_IsDirty)
	{
		VkCommandBuffer commandBuffer = pCommandBuffer->getCommandBuffer();

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = m_Size;
		vkCmdCopyBuffer(commandBuffer, m_PerFrameStagingBuffers[m_CurrentFrame].m_BufferHandle, m_BufferHandle, 1, &copyRegion);

		m_IsDirty = false;
	}
}

void VulkanConstantBuffer::bind(Material* material)
{	
    m_pRenderer->setConstantBuffer(this, m_Location);
}
