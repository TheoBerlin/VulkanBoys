#pragma once
#include "Vulkan/VulkanCommon.h"

//TODO: API - Independent enums

struct BufferParams
{
	VkDeviceSize			SizeInBytes		= 0;
	VkBufferUsageFlags		Usage			= 0;
	VkMemoryPropertyFlags	MemoryProperty	= 0;
	bool					IsExclusive		= false;
};

class IBuffer
{
public:
	DECL_INTERFACE(IBuffer);

	virtual bool init(const BufferParams& params) = 0;

	virtual void map(void** ppMemory) = 0;
	virtual void unmap() = 0;

	virtual uint64_t getSizeInBytes() const = 0;
};
