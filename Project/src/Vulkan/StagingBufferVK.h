#pragma once
#include "VulkanCommon.h"

#include "Core/Spinlock.h"

class DeviceVK;
class BufferVK;

class StagingBufferVK
{
public:
	StagingBufferVK(DeviceVK* pDevice);
	~StagingBufferVK();

	bool init(VkDeviceSize initalSizeInBytes);

	void* allocate(VkDeviceSize sizeInBytes);
	void reset();

	BufferVK* getBuffer() const { return m_pBuffer; }
	VkDeviceSize getCurrentOffset() const { return m_BufferOffset; }

private:
	DeviceVK* m_pDevice;
	BufferVK* m_pBuffer;
	BufferVK* m_pOldBuffer;
	uint8_t* m_pHostMemory;
	VkDeviceSize m_BufferOffset;
	Spinlock m_Lock; //TODO: We need to fix better thread saftey this is just a quick fix to avoid crashes
};
