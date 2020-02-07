#include "ImageViewVK.h"
#include "DeviceVK.h"
#include "ImageVK.h"

ImageViewVK::ImageViewVK(DeviceVK* pDevice, ImageVK* pImage)
	: m_pDevice(pDevice),
	m_pImage(pImage),
	m_ImageView(VK_NULL_HANDLE),
	m_Params()
{
}

ImageViewVK::~ImageViewVK()
{
	if (m_ImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_pDevice->getDevice(), m_ImageView, nullptr);
		m_ImageView = VK_NULL_HANDLE;
	}

	m_pImage = nullptr;
	m_pDevice = nullptr;
}

bool ImageViewVK::init(const ImageViewParams& params)
{
	VkImageViewCreateInfo imageViewInfo = {};
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.pNext = nullptr;
	imageViewInfo.flags = 0;
	imageViewInfo.image		= m_pImage->getImage();
	imageViewInfo.format	= m_pImage->getFormat();
	imageViewInfo.viewType	= params.Type;
	imageViewInfo.subresourceRange.aspectMask		= params.AspectFlags;
	imageViewInfo.subresourceRange.layerCount		= params.LayerCount;
	imageViewInfo.subresourceRange.baseArrayLayer	= params.FirstLayer;
	imageViewInfo.subresourceRange.levelCount		= params.MipLevels;
	imageViewInfo.subresourceRange.baseMipLevel		= params.FirstMipLevel;
	imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateImageView(m_pDevice->getDevice(), &imageViewInfo, nullptr, &m_ImageView), "vkCreateImageView failed");

	m_Params = params;
	D_LOG("--- ImageView: Vulkan ImageView created successfully");
	
	return true;
}
