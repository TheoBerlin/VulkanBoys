#pragma once
#include "../VertexBuffer.h"
#include "VulkanCommon.h"

class VulkanRenderer;

class VulkanVertexBuffer : public VertexBuffer
{
public:
	VulkanVertexBuffer(VulkanRenderer* pRenderer, size_t sizeInBytes, VertexBuffer::DATA_USAGE usage);
	~VulkanVertexBuffer();

	DECL_NO_COPY(VulkanVertexBuffer);

	virtual void setData(const void* data, size_t size, size_t offset) override;
	virtual void bind(size_t offset, size_t size, unsigned int location) override;
	virtual void unbind() override;
	virtual size_t getSize() override;

	VkBuffer getBuffer() { return m_Buffer; }
	size_t getOffset() { return m_Offset; }
	//This is not equal to the sizeInbytes (getSize), this is the size that is set in the bind call
	size_t getBoundSize() { return m_BoundSize;  }
private:
	VulkanRenderer* m_pRenderer;
	VkBuffer m_Buffer;
	uint64_t m_DataStartAddress;
	VkDeviceMemory m_Memory;
	size_t m_SizeInBytes;
	DATA_USAGE m_Usage;
	//Gets set when buffer is bound
	size_t m_Offset;
	size_t m_Location; 
	size_t m_BoundSize;
};

