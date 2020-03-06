#include "ImageVK.h"
#include "DeviceVK.h"

ImageVK::ImageVK(VkImage image, VkFormat format)
	: m_pDevice(nullptr),
	m_Image(image),
	m_Memory(VK_NULL_HANDLE),
	m_Params()
{
	m_Params.Format = format;
}

ImageVK::ImageVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_Image(VK_NULL_HANDLE),
	m_Memory(VK_NULL_HANDLE),
	m_Params()
{
}

ImageVK::~ImageVK()
{
	//If device is nullptr, we are not the owner of the memory
	if (m_pDevice != nullptr)
	{
		if (m_Image != VK_NULL_HANDLE)
		{
			vkDestroyImage(m_pDevice->getDevice(), m_Image, nullptr);
			m_Image = VK_NULL_HANDLE;
		}

		if (m_Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_pDevice->getDevice(), m_Memory, nullptr);
			m_Memory = VK_NULL_HANDLE;
		}
	}
}

bool ImageVK::init(const ImageParams& params)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.pNext					= nullptr;
	imageInfo.flags					= params.Flags;
	imageInfo.tiling				= VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage					= params.Usage;
	imageInfo.pQueueFamilyIndices	= nullptr;
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples				= params.Samples;
	imageInfo.format				= params.Format;
	imageInfo.imageType				= params.Type;
	imageInfo.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.mipLevels				= params.MipLevels;
	imageInfo.arrayLayers			= params.ArrayLayers;
	imageInfo.extent				= params.Extent;

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateImage(m_pDevice->getDevice(), &imageInfo, nullptr, &m_Image), "vkCreateImage failed");

	D_LOG("--- Image: Vulkan Image created successfully");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_pDevice->getDevice(), m_Image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(m_pDevice->getPhysicalDevice(), memRequirements.memoryTypeBits, params.MemoryProperty);

	VK_CHECK_RESULT_RETURN_FALSE(vkAllocateMemory(m_pDevice->getDevice(), &allocInfo, nullptr, &m_Memory), "Failed to allocate image memory");

	m_Params = params;
	D_LOG("--- Image: Allocated '%d' bytes for image", memRequirements.size);

	VK_CHECK_RESULT_RETURN_FALSE(vkBindImageMemory(m_pDevice->getDevice(), m_Image, m_Memory, 0), "Failed to bind image memory");

	return true;
}
