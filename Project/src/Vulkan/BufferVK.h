#pragma once
#include "VulkanCommon.h"

class DeviceVK;

struct BufferVkParams
{
	VkDeviceSize SizeInBytes = 0;
	VkBufferUsageFlags Usage = 0;
	VkMemoryPropertyFlags MemoryUsage = 0;
};

class BufferVK
{
public:
	BufferVK(DeviceVK* pDevice);
	~BufferVK();

	DECL_NO_COPY(BufferVK);

	bool create(const BufferVkParams& params);
	void map(void** ppMappedMemory);
	void unmap();

	VkBuffer getBuffer() const { return m_Buffer; }
	VkDeviceSize getSizeInBytes() const { return m_Params.SizeInBytes; }
private:
	DeviceVK* m_pDevice;
	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;
	BufferVkParams m_Params;
	bool m_IsMapped;
};

