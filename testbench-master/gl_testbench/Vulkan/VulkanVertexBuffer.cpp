#include "VulkanVertexBuffer.h"
#include "VulkanRenderer.h"

VulkanVertexBuffer::VulkanVertexBuffer(VulkanRenderer* pRenderer, size_t sizeInBytes, VertexBuffer::DATA_USAGE usage)
	: m_pRenderer(pRenderer),
	m_Buffer(VK_NULL_HANDLE),
	m_Memory(VK_NULL_HANDLE),
	m_SizeInBytes(sizeInBytes),
	m_Usage(usage)
{
	m_pRenderer->createBuffer(m_Buffer, m_Memory, sizeInBytes, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

VulkanVertexBuffer::~VulkanVertexBuffer()
{
	if (m_Buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(m_pRenderer->getDevice()->getDevice(), m_Buffer, nullptr);
		m_Buffer = VK_NULL_HANDLE;
	}

	if (m_Memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(m_pRenderer->getDevice()->getDevice(), m_Memory, nullptr);
		m_Memory = VK_NULL_HANDLE;
	}
}

void VulkanVertexBuffer::setData(const void* data, size_t size, size_t offset)
{
	//Make sure device is not using this buffer
	VkDevice device = m_pRenderer->getDevice()->getDevice();
	vkDeviceWaitIdle(device);

	//Write to buffer
	uint8_t* pCpu = nullptr;
	vkMapMemory(device, m_Memory, 0, m_SizeInBytes, 0, (void**)&pCpu);
		
	const uint8_t* pData = (const uint8_t*)data;
	memcpy(pCpu + offset, pData + offset, size);

	vkUnmapMemory(device, m_Memory);
}

void VulkanVertexBuffer::bind(size_t offset, size_t size, unsigned int location)
{
	m_Offset = offset;
	m_Location = location;
	m_BoundSize = size;
	m_pRenderer->setVertexBuffer(this, m_Location);
}

void VulkanVertexBuffer::unbind()
{
	m_pRenderer->setVertexBuffer(this, m_Location);
	m_BoundSize = 0;
	m_Location = 0;
	m_Offset = 0;
}

size_t VulkanVertexBuffer::getSize()
{
	return m_SizeInBytes;
}
