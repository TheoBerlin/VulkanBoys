#include "CommandPoolVK.h"
#include "CommandBufferVK.h"
#include "DeviceVK.h"
#include "InstanceVK.h"

CommandPoolVK::CommandPoolVK(DeviceVK* pDevice, InstanceVK* pInstance, uint32_t queueFamilyIndex)
	: m_pDevice(pDevice),
	m_pInstance(pInstance),
	m_QueueFamilyIndex(queueFamilyIndex),
	m_CommandPool(VK_NULL_HANDLE),
	m_ppCommandBuffers()
{
}

CommandPoolVK::~CommandPoolVK()
{
	for (CommandBufferVK* pCommandBuffer : m_ppCommandBuffers)
	{
		SAFEDELETE(pCommandBuffer);
	}

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
	createInfo.sType			= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext			= nullptr;
	createInfo.queueFamilyIndex = m_QueueFamilyIndex;
	createInfo.flags			= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateCommandPool(m_pDevice->getDevice(), &createInfo, nullptr, &m_CommandPool), "Create CommandPool Failed");

	D_LOG("--- CommandPool: Vulkan CommandPool created successfully");
	return true;
}

CommandBufferVK* CommandPoolVK::allocateCommandBuffer(VkCommandBufferLevel bufferLevel)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext					= nullptr;
	allocInfo.level					= bufferLevel;
	allocInfo.commandPool			= m_CommandPool;
	allocInfo.commandBufferCount	= 1;

	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkResult result = vkAllocateCommandBuffers(m_pDevice->getDevice(), &allocInfo, &commandBuffer);
	if (result != VK_SUCCESS)
	{
		LOG("vkAllocateCommandBuffers failed");
		return nullptr;
	}

	D_LOG("--- CommandPool: Vulkan CommandBuffer allocated successfully");
	
	CommandBufferVK* pCommandBuffer = DBG_NEW CommandBufferVK(m_pDevice, m_pInstance, commandBuffer);
	pCommandBuffer->finalize();

	m_ppCommandBuffers.emplace_back(pCommandBuffer);
	return pCommandBuffer;
}

void CommandPoolVK::freeCommandBuffer(CommandBufferVK** ppCommandBuffer)
{
	ASSERT(ppCommandBuffer != nullptr);

	VkCommandBuffer commandBuffer = (*ppCommandBuffer)->m_CommandBuffer;
	if (commandBuffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(m_pDevice->getDevice(), m_CommandPool, 1, &commandBuffer);
		(*ppCommandBuffer)->m_CommandBuffer = VK_NULL_HANDLE;

		D_LOG("--- CommandPool: Freed commandbuffer");
	}

	for (auto it = m_ppCommandBuffers.begin(); it != m_ppCommandBuffers.end(); it++)
	{
		if (*it == *ppCommandBuffer)
		{
			m_ppCommandBuffers.erase(it);
			break;
		}
	}

	SAFEDELETE(*ppCommandBuffer);
}

void CommandPoolVK::reset()
{
	VK_CHECK_RESULT(vkResetCommandPool(m_pDevice->getDevice(), m_CommandPool, 0), "Reset CommandPool Failed");
}

void CommandPoolVK::setName(const char* pName)
{
	if (pName)
	{
		if (m_pInstance->vkSetDebugUtilsObjectNameEXT)
		{
			VkDebugUtilsObjectNameInfoEXT info = {};
			info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			info.pNext = nullptr;
			info.objectType = VK_OBJECT_TYPE_COMMAND_POOL;
			info.objectHandle = (uint64_t)m_CommandPool;
			info.pObjectName = pName;
			m_pInstance->vkSetDebugUtilsObjectNameEXT(m_pDevice->getDevice(), &info);
		}
	}
}
