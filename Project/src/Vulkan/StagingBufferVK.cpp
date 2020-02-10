#include "StagingBufferVK.h"
#include "BufferVK.h"

StagingBufferVK::StagingBufferVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_pBuffer(nullptr),
	m_pHostMemory(nullptr),
	m_BufferOffset(0)
{
}

StagingBufferVK::~StagingBufferVK()
{
	SAFEDELETE(m_pBuffer);
	m_pDevice = nullptr;
}

bool StagingBufferVK::init(VkDeviceSize initalSizeInBytes)
{
	BufferParams params = {};
	params.Usage		= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	params.MemoryProperty	= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	params.SizeInBytes	= initalSizeInBytes;

	m_pBuffer = DBG_NEW BufferVK(m_pDevice);
	if (m_pBuffer->init(params))
	{
		m_pBuffer->map((void**)&m_pHostMemory);
		return true;
	}

	LOG("Failed to create Stack");
	return false;
}

void* StagingBufferVK::allocate(VkDeviceSize sizeInBytes)
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

void StagingBufferVK::reset()
{
	m_BufferOffset = 0;
}
