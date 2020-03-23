#pragma once
#include "VulkanCommon.h"

#include <vector>

class DeviceVK;
class InstanceVK;
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
	void reset();

	void setName(const char* pName);

private:
	DeviceVK* m_pDevice;
	std::vector<CommandBufferVK*> m_ppCommandBuffers;
	uint32_t m_QueueFamilyIndex;
	VkCommandPool m_CommandPool;
};