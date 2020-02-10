#pragma once
#include "Common/IBuffer.h"

#include "VulkanCommon.h"

class DeviceVK;

class BufferVK : public IBuffer
{
public:
	BufferVK(DeviceVK* pDevice);
	~BufferVK();

	DECL_NO_COPY(BufferVK);

	virtual bool init(const BufferParams& params) override;
	
	virtual void map(void** ppMappedMemory) override;
	virtual void unmap() override;

	virtual uint64_t getSizeInBytes() const override;
	
	VkBuffer getBuffer() const { return m_Buffer; }

private:
	DeviceVK* m_pDevice;
	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;
	BufferParams m_Params;
	bool m_IsMapped;
};

