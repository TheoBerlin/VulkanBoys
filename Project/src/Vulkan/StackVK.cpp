#include "StackVK.h"
#include "BufferVK.h"

StaginBufferVK::StaginBufferVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_pBuffer(nullptr),
	m_pHostMemory(nullptr),
	m_BufferOffset(0)
{
}

StaginBufferVK::~StaginBufferVK()
{
	SAFEDELETE(m_pBuffer);
	m_pDevice = nullptr;
}

bool StaginBufferVK::create(VkDeviceSize initalSizeInBytes)
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

void* StaginBufferVK::allocate(VkDeviceSize sizeInBytes)
{
	VkDeviceSize oldBufferOffset = m_BufferOffset;
	m_BufferOffset = m_BufferOffset + sizeInBytes;
	if (m_BufferOffset <= m_pBuffer->getSizeInBytes())
	{
		return (void*)(m_pHostMemory + oldBufferOffset);
	}
	else
	{
		//Restore old offset, maybe not neccessary??
		m_BufferOffset = oldBufferOffset;
	}

	D_LOG("-------- Error: Stack does not have enough space");

	//TODO: Here we need to reallocate the buffer

	return nullptr;
}

void StaginBufferVK::reset()
{
	m_BufferOffset = 0;
}
