#include "ReflectionProbeVK.h"
#include "TextureCubeVK.h"
#include "FrameBufferVK.h"
#include "ImageViewVK.h"

#include <algorithm>

#ifdef max
	#undef max
#endif

ReflectionProbeVK::ReflectionProbeVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_FrameBuffers(),
	m_ImageViews()
{
}

ReflectionProbeVK::~ReflectionProbeVK()
{
	ASSERT(m_FrameBuffers.size() == m_ImageViews.size());

	const uint32_t size = uint32_t(m_FrameBuffers.size());
	for (uint32_t i = 0; i < size; i++)
	{
		SAFEDELETE(m_FrameBuffers[i]);
		SAFEDELETE(m_ImageViews[i]);
	}
}

bool ReflectionProbeVK::initFromTextureCube(TextureCubeVK* pCubemap, RenderPassVK* pRenderPass)
{
	ImageVK* pImage = pCubemap->getImage();
	const uint32_t miplevels		= pCubemap->getMiplevels();
	const uint32_t frameBufferCount = miplevels * 6;

	ImageViewParams imageViewParams = {};
	imageViewParams.AspectFlags		= VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewParams.LayerCount		= 1;
	imageViewParams.MipLevels		= 1;
	imageViewParams.Type			= VK_IMAGE_VIEW_TYPE_2D;

	m_ImageViews.resize(frameBufferCount);
	m_FrameBuffers.resize(frameBufferCount);

	uint32_t size = pCubemap->getWidth();
	for (uint32_t mip = 0; mip < miplevels; mip++)
	{
		imageViewParams.FirstMipLevel = mip;

		for (uint32_t i = 0; i < 6; i++)
		{
			const uint32_t mipIndex = (mip * 6) + i;
			imageViewParams.FirstLayer = i;

			m_ImageViews[mipIndex] = DBG_NEW ImageViewVK(m_pDevice, pImage);
			if (!m_ImageViews[mipIndex]->init(imageViewParams))
			{
				return false;
			}

			m_FrameBuffers[mipIndex] = DBG_NEW FrameBufferVK(m_pDevice);
			m_FrameBuffers[mipIndex]->addColorAttachment(m_ImageViews[mipIndex]);
			if (!m_FrameBuffers[mipIndex]->finalize(pRenderPass, size, size))
			{
				return false;
			}
		}

		size = std::max(size / 2U, 1U);
	}

	return true;
}
