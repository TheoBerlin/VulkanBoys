#include "Texture2DVK.h"
#include "ImageVK.h"
#include "ImageViewVK.h"
#include "DeviceVK.h"
#include "CommandPoolVK.h"
#include "GraphicsContextVK.h"

#include "stb_image.h"
#include "BufferVK.h"
#include "ImageVK.h"
#include "CommandBufferVK.h"

Texture2DVK::Texture2DVK(IGraphicsContext* pContext) :
	m_pContext(reinterpret_cast<GraphicsContextVK*>(pContext)),
	m_pTextureImage(nullptr),
	m_pTextureImageView(nullptr),
	m_pTempCommandPool(nullptr),
	m_pTempCommandBuffer(nullptr)
{
	m_pTempCommandPool = new CommandPoolVK(m_pContext->getDevice(), m_pContext->getDevice()->getQueueFamilyIndices().graphicsFamily.value());
	m_pTempCommandPool->init();

	m_pTempCommandBuffer = m_pTempCommandPool->allocateCommandBuffer();
}

Texture2DVK::~Texture2DVK()
{
	SAFEDELETE(m_pTempCommandPool);
	SAFEDELETE(m_pTextureImage);
	SAFEDELETE(m_pTextureImageView);
}

bool Texture2DVK::loadFromFile(const std::string& filename)
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
	imageParams.Extent.depth = 1;
	imageParams.Extent.width = width;
	imageParams.Extent.height = height;
	imageParams.Format = VK_FORMAT_R8G8B8A8_UNORM;
	imageParams.Type = VK_IMAGE_TYPE_2D;
	imageParams.MipLevels = 1;
	imageParams.Samples = VK_SAMPLE_COUNT_1_BIT;
	imageParams.Usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageParams.ArrayLayers = 1;
	imageParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	
	DeviceVK* pDevice = m_pContext->getDevice();
	m_pTextureImage = new ImageVK(pDevice);
	m_pTextureImage->init(imageParams);

	m_pTempCommandBuffer->reset();
	m_pTempCommandBuffer->begin();
	m_pTempCommandBuffer->transitionImageLayout(m_pTextureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_pTempCommandBuffer->updateImage(pData, m_pTextureImage, width, height);
	m_pTempCommandBuffer->transitionImageLayout(m_pTextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_pTempCommandBuffer->end();
	
	pDevice->executeCommandBuffer(pDevice->getGraphicsQueue(), m_pTempCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
	pDevice->wait();

	ImageViewParams imageViewParams = {};
	imageViewParams.Type = VK_IMAGE_VIEW_TYPE_2D;
	imageViewParams.AspectFlags	= VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewParams.LayerCount	= 1;
	imageViewParams.FirstLayer	= 0;
	imageViewParams.MipLevels	= 1;
	imageViewParams.FirstMipLevel = 0;
	
	m_pTextureImageView = new ImageViewVK(pDevice, m_pTextureImage);
	m_pTextureImageView->init(imageViewParams);

	return true;
}
