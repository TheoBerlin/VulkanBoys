#include "StagingBufferVK.h"
#include "BufferVK.h"

#include <mutex>

StagingBufferVK::StagingBufferVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_pBuffer(nullptr),
	m_pOldBuffer(nullptr),
	m_pHostMemory(nullptr),
	m_BufferOffset(0)
{
}

StagingBufferVK::~StagingBufferVK()
{
	SAFEDELETE(m_pBuffer);
	SAFEDELETE(m_pOldBuffer);

	m_pDevice = nullptr;
}

bool StagingBufferVK::init(VkDeviceSize initalSizeInBytes)
{
	BufferParams params = {};
	params.Usage			= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	params.MemoryProperty	= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	params.SizeInBytes		= initalSizeInBytes;

	m_pBuffer = DBG_NEW BufferVK(m_pDevice);
	if (m_pBuffer->init(params))
	{
		m_pBuffer->map((void**)&m_pHostMemory);
		return true;
	}

	LOG("Failed to create StagingBuffer");
	return false;
}

void* StagingBufferVK::allocate(VkDeviceSize sizeInBytes)
{
	{
		//TODO: We need to fix better thread saftey this is just a quick fix to avoid crashes
		std::scoped_lock lock(m_Lock);

		VkDeviceSize oldBufferOffset = m_BufferOffset;
		m_BufferOffset = m_BufferOffset + sizeInBytes;
		if (m_BufferOffset <= m_pBuffer->getSizeInBytes())
		{
			return (void*)(m_pHostMemory + oldBufferOffset);
		}

		//Do not reallocate more than once for now
		ASSERT(m_pOldBuffer == nullptr);
		m_pOldBuffer = m_pBuffer;

		D_LOG("Reallocating StagingBuffer");

		m_pBuffer = DBG_NEW BufferVK(m_pDevice);
	}

	BufferParams params = {};
	params.Usage			= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	params.MemoryProperty	= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	params.SizeInBytes		= m_pBuffer->getSizeInBytes() + sizeInBytes + MB(1); 
	
	if (m_pBuffer->init(params))
	{
		m_pBuffer->map((void**)&m_pHostMemory);

		m_BufferOffset = 0;
		return allocate(sizeInBytes);
	}

	return nullptr;
}

void StagingBufferVK::reset()
{
	//TODO: We need to fix better thread saftey this is just a quick fix to avoid crashes
	std::scoped_lock lock(m_Lock);

	SAFEDELETE(m_pOldBuffer);
	m_BufferOffset = 0;
}
