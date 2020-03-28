#pragma once
#include "VulkanCommon.h"
#include "DeviceVK.h"

#include <vector>

class CommandBufferVK;

class CommandPoolVK
{
public:
	CommandPoolVK(DeviceVK* pDevice, uint32_t queueFamilyIndex);
	~CommandPoolVK();

	DECL_NO_COPY(CommandPoolVK);

	bool init();

	CommandBufferVK* allocateCommandBuffer(VkCommandBufferLevel bufferLevel);
	void freeCommandBuffer(CommandBufferVK** ppCommandBuffer);

	void setName(const char* pName);

	FORCEINLINE void reset()
	{
		vkResetCommandPool(m_pDevice->getDevice(), m_CommandPool, 0);
	}

private:
	DeviceVK* m_pDevice;
	std::vector<CommandBufferVK*> m_ppCommandBuffers;
	uint32_t m_QueueFamilyIndex;
	VkCommandPool m_CommandPool;
};