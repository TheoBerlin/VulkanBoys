#include "CommandPoolVK.h"
#include "CommandBufferVK.h"
#include "DeviceVK.h"

CommandPoolVK::CommandPoolVK(DeviceVK* pDevice, uint32_t queueFamilyIndex)
	: m_pDevice(pDevice),
	m_QueueFamilyIndex(queueFamilyIndex),
	m_CommandPool(VK_NULL_HANDLE)
{
}

CommandPoolVK::~CommandPoolVK()
{
	if (m_CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_pDevice->getDevice(), m_CommandPool, nullptr);
		m_CommandPool = VK_NULL_HANDLE;
	}

	m_pDevice = nullptr;
}

bool CommandPoolVK::init()
{
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.queueFamilyIndex = m_QueueFamilyIndex;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateCommandPool(m_pDevice->getDevice(), &createInfo, nullptr, &m_CommandPool), "Create CommandPool Failed");

	D_LOG("--- CommandPool: Vulkan CommandPool created successfully");
	return true;
}

CommandBufferVK* CommandPoolVK::allocateCommandBuffer()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkResult result = vkAllocateCommandBuffers(m_pDevice->getDevice(), &allocInfo, &commandBuffer);
	if (result != VK_SUCCESS)
	{
		LOG("vkAllocateCommandBuffers failed");
		return nullptr;
	}

	D_LOG("--- CommandPool: Vulkan CommandBuffer allocated successfully");
	
	CommandBufferVK* pCommandBuffer = new CommandBufferVK(m_pDevice);
	pCommandBuffer->finalize(commandBuffer);

	return pCommandBuffer;
}

void CommandPoolVK::reset()
{
	VK_CHECK_RESULT(vkResetCommandPool(m_pDevice->getDevice(), m_CommandPool, 0), "Reset CommandPool Failed");
}
