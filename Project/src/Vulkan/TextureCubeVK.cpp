#include "TextureCubeVK.h"
#include "ImageVK.h"
#include "ImageViewVK.h"

TextureCubeVK::TextureCubeVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_pImage(nullptr),
	m_pImageView(nullptr)
{
}

TextureCubeVK::~TextureCubeVK()
{
	SAFEDELETE(m_pImage);
	SAFEDELETE(m_pImageView);
}

bool TextureCubeVK::init(uint32_t width, uint32_t miplevels, ETextureFormat format)
{
	ImageParams imageParams = {};
	imageParams.Type			= VK_IMAGE_TYPE_2D;
	imageParams.Flags			= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	imageParams.Extent			= { width, width, 1 };
	imageParams.ArrayLayers		= 6;
	imageParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageParams.MipLevels		= miplevels;
	imageParams.Samples			= VK_SAMPLE_COUNT_1_BIT;
	imageParams.Usage			= VK_IMAGE_USAGE_SAMPLED_BIT;
	imageParams.Format			= convertFormat(format);

	m_pImage = DBG_NEW ImageVK(m_pDevice);
	if (!m_pImage->init(imageParams))
	{
		return false;
	}

	ImageViewParams imageViewParams = {};
	imageViewParams.Type			= VK_IMAGE_VIEW_TYPE_CUBE;
	imageViewParams.AspectFlags		= VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewParams.FirstMipLevel	= 0;
	imageViewParams.LayerCount		= 6;
	imageViewParams.MipLevels		= miplevels;
	imageViewParams.FirstLayer		= 0;

	m_pImageView = DBG_NEW ImageViewVK(m_pDevice, m_pImage);
	return m_pImageView->init(imageViewParams);
}
