#include "CopyHandlerVK.h"
#include "DeviceVK.h"
#include "CommandPoolVK.h"
#include "CommandBufferVK.h"
#include "GraphicsContextVK.h"
#include "ImageVK.h"

#include <mutex>
#include <algorithm>

#ifdef max
	#undef max
#endif

CopyHandlerVK::CopyHandlerVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_pTransferPool(),
	m_pGraphicsPool(),
	m_pTransferBuffers(),
	m_pGraphicsBuffers(),
	m_CurrentTransferBuffer(0),
	m_CurrentGraphicsBuffer(0)
{
}

CopyHandlerVK::~CopyHandlerVK()
{
	for (uint32_t i = 0; i < MAX_COMMAND_BUFFERS; i++)
	{
		SAFEDELETE(m_pGraphicsPool[i]);
		SAFEDELETE(m_pTransferPool[i]);
	}

	m_pDevice = nullptr;
}

bool CopyHandlerVK::init()
{
	for (uint32_t i = 0; i < MAX_COMMAND_BUFFERS; i++)
	{
		m_pGraphicsPool[i] = DBG_NEW CommandPoolVK(m_pDevice, m_pDevice->getQueueFamilyIndices().graphicsFamily.value());
		if (!m_pGraphicsPool[i]->init())
		{
			return false;
		}

		m_pTransferPool[i] = DBG_NEW CommandPoolVK(m_pDevice, m_pDevice->getQueueFamilyIndices().transferFamily.value());
		if (!m_pTransferPool[i]->init())
		{
			return false;
		}

		m_pTransferBuffers[i] = m_pTransferPool[i]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		if (!m_pTransferBuffers[i])
		{
			return false;
		}

		m_pGraphicsBuffers[i] = m_pGraphicsPool[i]->allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		if (!m_pGraphicsBuffers[i])
		{
			return false;
		}
	}

	return true;
}

void CopyHandlerVK::updateBuffer(BufferVK* pDestination, uint64_t destinationOffset, const void* pSource, uint64_t sizeInBytes)
{
	CommandBufferVK* pCommandBuffer = getNextTransferBuffer();
	{
		std::scoped_lock<Spinlock> lock(m_pTransferLocks[m_CurrentTransferBuffer]);

		pCommandBuffer->reset(true);
		pCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		pCommandBuffer->updateBuffer(pDestination, destinationOffset, pSource, sizeInBytes);
		pCommandBuffer->end();

		submitTransferBuffer(pCommandBuffer);
	}
}

void CopyHandlerVK::copyBuffer(BufferVK* pSource, uint64_t sourceOffset, BufferVK* pDestination, uint64_t destinationOffset, uint64_t sizeInBytes)
{
	CommandBufferVK* pCommandBuffer = getNextTransferBuffer();
	{
		std::scoped_lock<Spinlock> lock(m_pTransferLocks[m_CurrentTransferBuffer]);

		pCommandBuffer->reset(true);
		pCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		pCommandBuffer->copyBuffer(pSource, sourceOffset, pDestination, destinationOffset, sizeInBytes);
		pCommandBuffer->end();

		submitTransferBuffer(pCommandBuffer);
	}
}

void CopyHandlerVK::updateImage(const void* pPixelData, ImageVK* pImage, uint32_t width, uint32_t height, uint32_t pixelStride, VkImageLayout initalLayout, VkImageLayout finalLayout, uint32_t miplevel, uint32_t layer)
{
	CommandBufferVK* pCommandBuffer = getNextGraphicsBuffer();
	{
		std::scoped_lock<Spinlock> lock(m_pGraphicsLocks[m_CurrentGraphicsBuffer]);

		pCommandBuffer->reset(true);
		pCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		//Insert barrier if we need to
		if (initalLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			pCommandBuffer->transitionImageLayout(pImage, initalLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, pImage->getMiplevelCount(), layer, 1);
		}

		pCommandBuffer->updateImage(pPixelData, pImage, width, height, pixelStride, miplevel, layer);

		//Insert barrier if we need to
		if (finalLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			pCommandBuffer->transitionImageLayout(pImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout, 0, pImage->getMiplevelCount(), layer, 1);
		}

		pCommandBuffer->end();

		submitGraphicsBuffer(pCommandBuffer);
	}
}

void CopyHandlerVK::copyBufferToImage(BufferVK* pSource, VkDeviceSize sourceOffset, ImageVK* pImage, uint32_t width, uint32_t height, uint32_t miplevel, uint32_t layer)
{
	CommandBufferVK* pCommandBuffer = getNextGraphicsBuffer();
	{
		std::scoped_lock<Spinlock> lock(m_pGraphicsLocks[m_CurrentGraphicsBuffer]);

		pCommandBuffer->reset(true);
		pCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		pCommandBuffer->copyBufferToImage(pSource, sourceOffset, pImage, width, height, miplevel, layer);
		pCommandBuffer->end();

		submitGraphicsBuffer(pCommandBuffer);
	}
}

void CopyHandlerVK::generateMips(ImageVK* pImage)
{
	//D_LOG("CopyHandlerVK::generateMips");

	CommandBufferVK* pCommandBuffer = getNextGraphicsBuffer();
	{
		std::scoped_lock<Spinlock> lock(m_pGraphicsLocks[m_CurrentGraphicsBuffer]);

		pCommandBuffer->reset(true);
		pCommandBuffer->begin(nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		const uint32_t miplevelCount = pImage->getMiplevelCount();
		pCommandBuffer->transitionImageLayout(pImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, miplevelCount, 0, 1);

		VkExtent2D destinationExtent = {};
		VkExtent2D sourceExtent = { pImage->getExtent().width, pImage->getExtent().height };
		for (uint32_t i = 1; i < miplevelCount; i++)
		{
			destinationExtent = { std::max(sourceExtent.width / 2U, 1u), std::max(sourceExtent.height / 2U, 1U) };

			pCommandBuffer->transitionImageLayout(pImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, i - 1, 1, 0, 1);
			pCommandBuffer->blitImage2D(pImage, i - 1, sourceExtent, pImage, i, destinationExtent);
			sourceExtent = destinationExtent;
		}

		pCommandBuffer->transitionImageLayout(pImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, miplevelCount - 1, 1, 0, 1);
		pCommandBuffer->transitionImageLayout(pImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, miplevelCount, 0, 1);
		pCommandBuffer->end();

		submitGraphicsBuffer(pCommandBuffer);
	}
}

CommandBufferVK* CopyHandlerVK::getNextTransferBuffer()
{
	CommandBufferVK* pCommandBuffer = m_pTransferBuffers[m_CurrentTransferBuffer];
	m_CurrentTransferBuffer = (m_CurrentTransferBuffer+1) % MAX_COMMAND_BUFFERS;

	return pCommandBuffer;
}

CommandBufferVK* CopyHandlerVK::getNextGraphicsBuffer()
{
	CommandBufferVK* pCommandBuffer = m_pGraphicsBuffers[m_CurrentGraphicsBuffer];
	m_CurrentGraphicsBuffer = (m_CurrentGraphicsBuffer + 1) % MAX_COMMAND_BUFFERS;
	return pCommandBuffer;
}

void CopyHandlerVK::submitTransferBuffer(CommandBufferVK* pCommandBuffer)
{
	m_pDevice->executeTransfer(pCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
}

void CopyHandlerVK::submitGraphicsBuffer(CommandBufferVK* pCommandBuffer)
{
	m_pDevice->executeGraphics(pCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
}
