#pragma once
#include "VulkanCommon.h"

class DeviceVK;
class BufferVK;

class StackVK
{
public:
	StackVK(DeviceVK* pDevice);
	~StackVK();

	bool create(VkDeviceSize initalSizeInBytes);

	void* allocate(VkDeviceSize sizeInBytes);
	void reset();

	BufferVK* getBuffer() const { return m_pBuffer; }
	VkDeviceSize getCurrentOffset() const { return m_BufferOffset; }
private:
	DeviceVK* m_pDevice;
	BufferVK* m_pBuffer;
	uint8_t* m_pHostMemory;
	VkDeviceSize m_BufferOffset;
};
