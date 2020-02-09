#include "StackVK.h"
#include "BufferVK.h"

StackVK::StackVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_pBuffer(nullptr),
	m_pHostMemory(nullptr),
	m_BufferOffset(0)
{
}

StackVK::~StackVK()
{
	SAFEDELETE(m_pBuffer);
	m_pDevice = nullptr;
}

bool StackVK::create(VkDeviceSize initalSizeInBytes)
{
	BufferParams params = {};
	params.Usage		= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	params.MemoryProperty	= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	params.SizeInBytes	= initalSizeInBytes;

	m_pBuffer = new BufferVK(m_pDevice);
	if (m_pBuffer->create(params))
	{
		m_pBuffer->map((void**)&m_pHostMemory);
		return true;
	}

	LOG("Failed to create Stack");
	return false;
}

void* StackVK::allocate(VkDeviceSize sizeInBytes)
{
	VkDeviceSize newBufferOffset = m_BufferOffset + sizeInBytes;
	if (newBufferOffset <= m_pBuffer->getSizeInBytes())
	{
		m_BufferOffset = newBufferOffset;
		return (void*)(m_pHostMemory + m_BufferOffset);
	}

	D_LOG("-------- Error: Stack does not have enough space");

	//TODO: Here we need to reallocate the buffer

	return nullptr;
}

void StackVK::reset()
{
	m_BufferOffset = 0;
}
