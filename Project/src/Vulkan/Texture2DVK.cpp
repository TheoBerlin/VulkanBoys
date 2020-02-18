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

bool Texture2DVK::initFromFile(const std::string& filename, bool generateMips)
{
	int texWidth, texHeight, bpp;
	unsigned char* pPixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &bpp, STBI_rgb_alpha);
	
	if (pPixels == nullptr)
	{
		LOG("Error loading texture file: %s", filename.c_str());
		return false;
	}

	LOG("-- LOADED TEXTURE: %s", filename.c_str());

	bool result = initFromMemory(pPixels, texWidth, texHeight, generateMips);
	stbi_image_free(pPixels);
	
	return result;
}

bool Texture2DVK::initFromMemory(const void* pData, uint32_t width, uint32_t height, bool generateMips)
{
	uint32_t miplevels = 1U;
	if (generateMips)
	{
		miplevels = std::floor(std::log2(std::max(width, height))) + 1U;
	}

	ImageParams imageParams = {};
	imageParams.Type			= VK_IMAGE_TYPE_2D;
	imageParams.Format			= VK_FORMAT_R8G8B8A8_UNORM;
	imageParams.Extent.depth	= 1;
	imageParams.Extent.width	= width;
	imageParams.Extent.height	= height;
	imageParams.MipLevels		= miplevels;
	imageParams.Samples			= VK_SAMPLE_COUNT_1_BIT;
	imageParams.ArrayLayers		= 1;
	imageParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageParams.Usage			= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	
	if (generateMips)
	{
		imageParams.Usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	
	m_pTextureImage = DBG_NEW ImageVK(m_pDevice);
	if (!m_pTextureImage->init(imageParams))
	{
		return false;
	}

	CopyHandlerVK* pCopyHandler = m_pDevice->getCopyHandler();
	pCopyHandler->updateImage(pData, m_pTextureImage, width, height, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0);

	if (generateMips)
	{
		pCopyHandler->generateMips(m_pTextureImage);
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
