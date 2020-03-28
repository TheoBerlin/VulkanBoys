#pragma once

#include "Common/IImage.h"
#include "VulkanCommon.h"

class DeviceVK;

struct ImageParams
{
	VkImageType				Type;
	VkImageCreateFlags		Flags;
	VkSampleCountFlagBits	Samples;
	VkImageUsageFlags		Usage;
	VkMemoryPropertyFlags	MemoryProperty;
	VkFormat				Format;
	VkExtent3D				Extent;
	uint32_t				MipLevels;
	uint32_t				ArrayLayers;
};

class ImageVK : public IImage
{
public:
	ImageVK(VkImage image, VkFormat format);
	ImageVK(DeviceVK* pDevice);
	~ImageVK();

	DECL_NO_COPY(ImageVK);

	bool init(const ImageParams& params);

	VkImage getImage() const { return m_Image; }
	VkFormat getFormat() const { return m_Params.Format; }
	VkExtent3D getExtent() const { return m_Params.Extent; }
	uint32_t getMiplevelCount() const { return m_Params.MipLevels; }
	uint32_t getArrayLayers() const { return m_Params.ArrayLayers;  }

private:
	DeviceVK* m_pDevice;
	VkImage m_Image;
	VkDeviceMemory m_Memory;
	ImageParams m_Params;
};