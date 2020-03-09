#include "GBufferVK.h"
#include "ImageVK.h"
#include "DeviceVK.h"
#include "ImageViewVK.h"
#include "FrameBufferVK.h"

GBufferVK::GBufferVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_ColorFormats(),
	m_ColorImages(),
	m_ColorImageViews(),
	m_pFrameBuffer(nullptr),
	m_DepthFormat(VK_FORMAT_UNDEFINED),
	m_pDepthImage(nullptr),
	m_pDepthImageView(nullptr)
{
}

GBufferVK::~GBufferVK()
{
	releaseBuffers();
}

void GBufferVK::resize(uint32_t width, uint32_t height)
{
	m_pDevice->wait();
	releaseBuffers();

	m_Extent = { width, height };

	createImages();
	createImageViews();

	createFrameBuffer(m_pRenderPass);
}

void GBufferVK::addColorAttachmentFormat(VkFormat format)
{
	m_ColorFormats.emplace_back(format);
}

void GBufferVK::setDepthAttachmentFormat(VkFormat format)
{
	m_DepthFormat = format;
}

bool GBufferVK::finalize(RenderPassVK* pRenderPass, uint32_t width, uint32_t height)
{
	m_Extent = { width, height };
	m_pRenderPass = pRenderPass;

	if (!createImages())
	{
		return false;
	}

	if (!createImageViews())
	{
		return false;
	}

	return createFrameBuffer(m_pRenderPass);
}

void GBufferVK::releaseBuffers()
{
	SAFEDELETE(m_pFrameBuffer);

	for (ImageViewVK* pImageView : m_ColorImageViews)
	{
		SAFEDELETE(pImageView);
	}
	m_ColorImageViews.clear();

	for (ImageVK* pImage : m_ColorImages)
	{
		SAFEDELETE(pImage);
	}
	m_ColorImages.clear();
	
	SAFEDELETE(m_pDepthImage);
	SAFEDELETE(m_pDepthImageView);
}

bool GBufferVK::createImages()
{
	ImageParams imageParams = {};
	imageParams.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageParams.Extent = { m_Extent.width, m_Extent.height, 1 };
	imageParams.ArrayLayers = 1;
	imageParams.MemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageParams.MipLevels = 1;
	imageParams.Samples = VK_SAMPLE_COUNT_1_BIT;
	imageParams.Type = VK_IMAGE_TYPE_2D;
	for (VkFormat format : m_ColorFormats)
	{
		imageParams.Format = format;

		ImageVK* pImage = DBG_NEW ImageVK(m_pDevice);
		if (!pImage->init(imageParams))
		{
			return false;
		}

		m_ColorImages.emplace_back(pImage);
	}

	imageParams.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageParams.Format = m_DepthFormat;

	m_pDepthImage = DBG_NEW ImageVK(m_pDevice);
	return m_pDepthImage->init(imageParams);
}

bool GBufferVK::createImageViews()
{
	ImageViewParams imageViewParams = {};
	imageViewParams.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewParams.FirstLayer = 0;
	imageViewParams.LayerCount = 1;
	imageViewParams.FirstMipLevel = 0;
	imageViewParams.MipLevels = 1;
	imageViewParams.Type = VK_IMAGE_VIEW_TYPE_2D;
	for (ImageVK* pImage : m_ColorImages)
	{
		ImageViewVK* pImageView = DBG_NEW ImageViewVK(m_pDevice, pImage);
		if (!pImageView->init(imageViewParams))
		{
			return false;
		}

		m_ColorImageViews.emplace_back(pImageView);
	}

	m_pDepthImageView = DBG_NEW ImageViewVK(m_pDevice, m_pDepthImage);
	if (m_DepthFormat == VK_FORMAT_D24_UNORM_S8_UINT || m_DepthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || m_DepthFormat == VK_FORMAT_D32_SFLOAT)
	{
		imageViewParams.AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else
	{
		imageViewParams.AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	return m_pDepthImageView->init(imageViewParams);
}

bool GBufferVK::createFrameBuffer(RenderPassVK* pRenderPass)
{	
	m_pFrameBuffer = DBG_NEW FrameBufferVK(m_pDevice);
	for (ImageViewVK* pImageView : m_ColorImageViews)
	{
		m_pFrameBuffer->addColorAttachment(pImageView);
	}
	m_pFrameBuffer->setDepthStencilAttachment(m_pDepthImageView);
	return m_pFrameBuffer->finalize(pRenderPass, m_Extent.width, m_Extent.height);
}
