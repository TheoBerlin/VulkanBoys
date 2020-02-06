#include "BufferVK.h"
#include "DeviceVK.h"

BufferVK::BufferVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_Buffer(VK_NULL_HANDLE),
	m_Memory(VK_NULL_HANDLE),
	m_Params(),
	m_IsMapped(false)
{
}

BufferVK::~BufferVK()
{
	if (m_IsMapped)
	{
		unmap();
	}

	if (m_Buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(m_pDevice->getDevice(), m_Buffer, nullptr);
		m_Buffer = VK_NULL_HANDLE;
	}

	if (m_Memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(m_pDevice->getDevice(), m_Memory, nullptr);
		m_Memory = VK_NULL_HANDLE;
	}

	m_pDevice = nullptr;
}

bool BufferVK::create(const BufferVkParams& params)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext	= nullptr;
	bufferInfo.flags	= 0;
	bufferInfo.size		= params.SizeInBytes;
	bufferInfo.usage	= params.Usage;
	bufferInfo.queueFamilyIndexCount	= 0;
	bufferInfo.pQueueFamilyIndices		= nullptr;
	bufferInfo.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateBuffer(m_pDevice->getDevice(), &bufferInfo, nullptr, &m_Buffer), "Failed to create buffer");
	
	m_Params = params;
	D_LOG("Created Buffer sizeInBytes=%d\n", params.SizeInBytes);

	VkMemoryRequirements memRequirements = {};
	vkGetBufferMemoryRequirements(m_pDevice->getDevice(), m_Buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.allocationSize	= memRequirements.size;
	allocInfo.memoryTypeIndex	= findMemoryType(m_pDevice->getPhysicalDevice(), memRequirements.memoryTypeBits, params.SizeInBytes);

	VK_CHECK_RESULT_RETURN_FALSE(vkAllocateMemory(m_pDevice->getDevice(), &allocInfo, nullptr, &m_Memory), "Failed to allocate memory for buffer");

	vkBindBufferMemory(m_pDevice->getDevice(), m_Buffer, m_Memory, 0);
	LOG("Allocated '%d' bytes for buffer\n", memRequirements.size);
}

void BufferVK::map(void** ppMappedMemory)
{
	assert((ppMappedMemory != nullptr) && (m_Params.MemoryUsage & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
	
	if (!m_IsMapped)
	{
		VK_CHECK_RESULT(vkMapMemory(m_pDevice->getDevice(), m_Memory, 0, m_Params.SizeInBytes, 0, ppMappedMemory), "MapMemory Failed");
		m_IsMapped = true;
	}
}

void BufferVK::unmap()
{
	vkUnmapMemory(m_pDevice->getDevice(), m_Memory);
	m_IsMapped;
}
