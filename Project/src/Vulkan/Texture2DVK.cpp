#include "Texture2DVK.h"
#include "ImageVK.h"
#include "ImageViewVK.h"
#include "DeviceVK.h"
#include "CommandPoolVK.h"
#include "CopyHandlerVK.h"
#include "GraphicsContextVK.h"

#include "stb_image.h"
#include "BufferVK.h"
#include "ImageVK.h"
#include "CommandBufferVK.h"

#ifdef max
	#undef max
#endif

Texture2DVK::Texture2DVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_pTextureImage(nullptr),
	m_pTextureImageView(nullptr)
{
}

Texture2DVK::~Texture2DVK()
{
	SAFEDELETE(m_pTextureImage);
	SAFEDELETE(m_pTextureImageView);
}

bool Texture2DVK::initFromFile(const std::string& filename, ETextureFormat format, bool generateMips)
{
	int texWidth = 0; 
	int texHeight = 0;
	int bpp = 0;

	void* pPixels = nullptr;
	if (format == ETextureFormat::FORMAT_R8G8B8A8_UNORM)
	{
		pPixels = (void*)stbi_load(filename.c_str(), &texWidth, &texHeight, &bpp, STBI_rgb_alpha);
	}
	else if (format == ETextureFormat::FORMAT_R32G32B32A32_FLOAT)
	{
		pPixels = (void*)stbi_loadf(filename.c_str(), &texWidth, &texHeight, &bpp, STBI_rgb_alpha);
	}
	else
	{
		LOG("Error format not supported");
		return false;
	}
	
	if (pPixels == nullptr)
	{
		LOG("Error loading texture file: %s", filename.c_str());
		return false;
	}

	LOG("-- LOADED TEXTURE: %s", filename.c_str());

	bool result = initFromMemory(pPixels, texWidth, texHeight, format, 0, generateMips);
	stbi_image_free(pPixels);
	
	return result;
}

bool Texture2DVK::initFromMemory(const void* pData, uint32_t width, uint32_t height, ETextureFormat format, uint32_t usageFlags, bool generateMips)
{
	uint32_t miplevels = 1U;
	if (generateMips)
	{
		miplevels = std::floor(std::log2(std::max(width, height))) + 1U;
	}

	ImageParams imageParams = {};
	imageParams.Type			= VK_IMAGE_TYPE_2D;
	imageParams.Extent.depth	= 1;
	imageParams.Extent.width	= width;
	imageParams.Extent.height	= height;
	imageParams.MipLevels		= miplevels;
	imageParams.Samples			= VK_SAMPLE_COUNT_1_BIT;
	imageParams.ArrayLayers		= 1;
	imageParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageParams.Usage			= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | usageFlags;
	imageParams.Format			= convertFormat(format);
	
	if (generateMips)
	{
		imageParams.Usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	
	m_pTextureImage = DBG_NEW ImageVK(m_pDevice);
	if (!m_pTextureImage->init(imageParams))
	{
		return false;
	}

	if (pData)
	{
		uint32_t pixelStride = textureFormatStride(format);

		CopyHandlerVK* pCopyHandler = m_pDevice->getCopyHandler();
		pCopyHandler->updateImage(pData, m_pTextureImage, width, height, pixelStride, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0);

		if (generateMips)
		{
			pCopyHandler->generateMips(m_pTextureImage);
		}
	}

	ImageViewParams imageViewParams = {};
	imageViewParams.Type			= VK_IMAGE_VIEW_TYPE_2D;
	imageViewParams.AspectFlags		= VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewParams.LayerCount		= 1;
	imageViewParams.FirstLayer		= 0;
	imageViewParams.MipLevels		= miplevels;
	imageViewParams.FirstMipLevel	= 0;
	
	m_pTextureImageView = DBG_NEW ImageViewVK(m_pDevice, m_pTextureImage);
	return m_pTextureImageView->init(imageViewParams);
}
