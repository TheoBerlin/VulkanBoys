#pragma once
#include "VulkanCommon.h"

class DeviceVK;

struct ImageParams
{
	VkImageType Type;
	VkSampleCountFlagBits Samples;
	VkImageUsageFlags Usage;
	VkMemoryPropertyFlags MemoryProperty;
	VkFormat Format;
	VkExtent3D Extent;
	uint32_t MipLevels;
	uint32_t ArrayLayers;
};

class ImageVK
{
public:
	ImageVK(VkImage image, VkFormat format);
	ImageVK(DeviceVK* pDevice);
	~ImageVK();

	DECL_NO_COPY(ImageVK);

	bool init(const ImageParams& params);

	VkImage getImage() const { return m_Image; }
	VkFormat getFormat() const { return m_Params.Format; }

private:
	DeviceVK* m_pDevice;
	VkImage m_Image;
	VkDeviceMemory m_Memory;
	ImageParams m_Params;
};