#pragma once

#include "Common/IImageView.h"
#include "VulkanCommon.h"

class ImageVK;
class DeviceVK;

struct ImageViewParams
{
	VkImageViewType Type;
	VkImageAspectFlags AspectFlags;
	uint32_t LayerCount;
	uint32_t FirstLayer;
	uint32_t MipLevels;
	uint32_t FirstMipLevel;
};

class ImageViewVK : public IImageView
{
public:
	ImageViewVK(DeviceVK* pDevice, ImageVK* pImage);
	~ImageViewVK();

	DECL_NO_COPY(ImageViewVK);

	bool init(const ImageViewParams& params);

	VkImageView getImageView() const { return m_ImageView; }

private:
	DeviceVK* m_pDevice;
	ImageVK* m_pImage;
	VkImageView m_ImageView;
	ImageViewParams m_Params;
};
