#include "VulkanVertexBuffer.h"

VulkanVertexBuffer::VulkanVertexBuffer(VulkanRenderer* pRenderer, size_t sizeInBytes, VertexBuffer::DATA_USAGE usage)
	: m_pRenderer(pRenderer),
	m_Buffer(VK_NULL_HANDLE),
	m_Memory(VK_NULL_HANDLE),
	m_Usage(usage)
{
	//uint64_t alignment = m_pRenderer->getDevice()->getPhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;
	//m_SizeInBytes = alignment * sizeInBytes;
	//m_pRenderer->createBuffer(m_Buffer, m_Memory, m_SizeInBytes, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	//void* pDataNext = nullptr;
	//vkMapMemory(pRenderer->getDevice()->getDevice(), m_Memory, 0, m_SizeInBytes, 0, (void**)&pDataNext);
	//m_DataStartAddress = (uint64_t)pDataNext;
}

VulkanVertexBuffer::~VulkanVertexBuffer()
{
	//if (m_pRenderer->getDevice()->getDevice() != VK_NULL_HANDLE)
	//	vkDeviceWaitIdle(m_pRenderer->getDevice()->getDevice());
	//
	//if (m_Buffer != VK_NULL_HANDLE)
	//{
	//	vkDestroyBuffer(m_pRenderer->getDevice()->getDevice(), m_Buffer, nullptr);
	//	m_Buffer = VK_NULL_HANDLE;
	//}

	//if (m_Memory != VK_NULL_HANDLE)
	//{
	//	vkUnmapMemory(m_pRenderer->getDevice()->getDevice(), m_Memory);
	//	vkFreeMemory(m_pRenderer->getDevice()->getDevice(), m_Memory, nullptr);
	//	m_Memory = VK_NULL_HANDLE;
	//}
}

void VulkanVertexBuffer::setData(const void* data, size_t size, size_t offset)
{
	////Make sure device is not using this buffer
	//VkDevice device = m_pRenderer->getDevice()->getDevice();
	//vkDeviceWaitIdle(device);

	//uint64_t alignment = m_pRenderer->getDevice()->getPhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;
	//uint64_t padding = (-offset & (alignment - 1));
	//uint64_t newOffset = (offset / size) * (size + padding);
	//uint64_t dataWriteAddress = ((m_DataStartAddress + newOffset) + (alignment - 1)) & -alignment;
	//
	//const uint8_t* pData = (const uint8_t*)data;
	//memcpy((void*)dataWriteAddress, pData, size);
}

void VulkanVertexBuffer::bind(size_t offset, size_t size, unsigned int location)
{
	//uint64_t alignment = m_pRenderer->getDevice()->getPhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;
	//uint64_t padding = (-offset & (alignment - 1));
	//uint64_t newOffset = (offset / size) * (size + padding);
	//m_Offset = (((offset / size) * (size + padding)) + (alignment - 1)) & -alignment;
	//m_Location = location;
	//m_BoundSize = size;
	//m_pRenderer->setVertexBuffer(this, m_Location);
}

void VulkanVertexBuffer::unbind()
{
	//m_pRenderer->setVertexBuffer(nullptr, m_Location);
	//m_BoundSize = 0;
	//m_Location = 0;
	//m_Offset = 0;
}

size_t VulkanVertexBuffer::getSize()
{
	return m_SizeInBytes;
}
