#include "ReflectionProbeVK.h"
#include "TextureCubeVK.h"
#include "FrameBufferVK.h"
#include "ImageViewVK.h"

ReflectionProbeVK::ReflectionProbeVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_pFrameBuffers(),
	m_ppImageViews()
{
	for (uint32_t i = 0; i < 6; i++)
	{
		m_pFrameBuffers[i] = nullptr;
		m_ppImageViews[i] = nullptr;
	}
}

ReflectionProbeVK::~ReflectionProbeVK()
{
	for (uint32_t i = 0; i < 6; i++)
	{
		SAFEDELETE(m_pFrameBuffers[i]);
		SAFEDELETE(m_ppImageViews[i]);
	}
}

bool ReflectionProbeVK::initFromTextureCube(TextureCubeVK* pCubemap, RenderPassVK* pRenderPass)
{
	ImageVK* pImage = pCubemap->getImage();
	uint32_t width	= pCubemap->getWidth();

	ImageViewParams imageViewParams = {};
	imageViewParams.AspectFlags		= VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewParams.FirstMipLevel	= 0;
	imageViewParams.LayerCount		= 1;
	imageViewParams.MipLevels		= 1;
	imageViewParams.Type			= VK_IMAGE_VIEW_TYPE_2D;

	for (uint32_t i = 0; i < 6; i++)
	{
		imageViewParams.FirstLayer = i;

		m_ppImageViews[i] = DBG_NEW ImageViewVK(m_pDevice, pImage);
		if (!m_ppImageViews[i]->init(imageViewParams))
		{
			return false;
		}

		m_pFrameBuffers[i] = DBG_NEW FrameBufferVK(m_pDevice);
		m_pFrameBuffers[i]->addColorAttachment(m_ppImageViews[i]);
		if (!m_pFrameBuffers[i]->finalize(pRenderPass, width, width))
		{
			return false;
		}
	}

	return true;
}
