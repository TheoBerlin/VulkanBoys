#include "Texture2DVK.h"
#include "DeviceVK.h"
#include "ContextVK.h"
#include "ImageVK.h"
#include "CommandPoolVK.h"

#include "stb_image.h"
#include "BufferVK.h"
#include "ImageVK.h"
#include "CommandBufferVK.h"

Texture2DVK::Texture2DVK(IContext* pContext) :
	m_pContext(reinterpret_cast<ContextVK*>(pContext)),
	m_pTextureImage(nullptr),
	m_pTextureImageView(nullptr),
	m_pTempCommandPool(nullptr),
	m_pTempCommandBuffer(nullptr)
{
	m_pTempCommandPool = new CommandPoolVK(m_pContext->getDevice(), m_pContext->getDevice()->getQueueFamilyIndices().transferFamily.value());
	m_pTempCommandPool->init();

	m_pTempCommandBuffer = m_pTempCommandPool->allocateCommandBuffer();
}

Texture2DVK::~Texture2DVK()
{
	if (m_pTempCommandPool != nullptr)
	{
		delete m_pTempCommandPool;
		m_pTempCommandPool = nullptr;
	}

	if (m_pTextureImage != nullptr)
	{
		delete m_pTextureImage;
		m_pTextureImage = nullptr;
	}

	if (m_pTextureImageView != nullptr)
	{
		delete m_pTextureImageView;
		m_pTextureImageView = nullptr;
	}

	//if (m_TextureImage != VK_NULL_HANDLE) 
	//{
	//	vkDestroyImage(m_pContext->getDevice()->getDevice(), m_TextureImage, nullptr);
	//	m_TextureImage = VK_NULL_HANDLE;
	//}

	//if (m_TextureImageView != VK_NULL_HANDLE) 
	//{
	//	vkDestroyImageView(m_pContext->getDevice()->getDevice(), m_TextureImageView, nullptr);
	//	m_TextureImageView = VK_NULL_HANDLE;
	//}

	//if (m_TextureImageMemory != VK_NULL_HANDLE)
	//{
	//	vkFreeMemory(m_pContext->getDevice()->getDevice(), m_TextureImageMemory, nullptr);
	//	m_TextureImageMemory = VK_NULL_HANDLE;
	//}
}

bool Texture2DVK::loadFromFile(std::string filename)
{
	int texWidth, texHeight, bpp;
	unsigned char* pPixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &bpp, STBI_rgb_alpha);
	
	if (pPixels == nullptr)
	{
		std::cerr << "Error loading texture file: " << filename.c_str() << std::endl;
		return false;
	}

	loadFromMemory(pPixels, texWidth, texHeight);
	stbi_image_free(pPixels);
	return true;
}

bool Texture2DVK::loadFromMemory(const void* pData, uint32_t width, uint32_t height)
{
	// Transfer staging buffer contents to the final texture buffer
	ImageParams imageParams = {};
	imageParams.Extent.width = 1;
	imageParams.Extent.width = width;
	imageParams.Extent.height = height;
	imageParams.Format = VK_FORMAT_R8G8B8A8_UNORM;
	imageParams.Type = VK_IMAGE_TYPE_2D;
	imageParams.MipLevels = 1;
	imageParams.Samples = VK_SAMPLE_COUNT_1_BIT;
	imageParams.Usage = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageParams.ArrayLayers = 1;
	imageParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	
	m_pTextureImage = reinterpret_cast<ImageVK*>(m_pContext->createImage());
	m_pTextureImage->init(imageParams);

	m_pTempCommandBuffer->begin();
	m_pTempCommandBuffer->transitionImageLayout(m_pTextureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_pTempCommandBuffer->updateImage(pData, m_pTextureImage, width, height);
	m_pTempCommandBuffer->transitionImageLayout(m_pTextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_pTempCommandBuffer->end();
	
	ImageViewParams imageViewParams = {};
	imageViewParams.Type = VK_IMAGE_VIEW_TYPE_2D;
	imageViewParams.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewParams.LayerCount = 1;
	imageViewParams.FirstLayer = 0;
	imageViewParams.MipLevels = 1;
	imageViewParams.FirstMipLevel = 0;
	
	m_pTextureImageView = reinterpret_cast<ImageViewVK*>(m_pContext->createImageView());
	m_pTextureImageView->init(imageViewParams);
}
