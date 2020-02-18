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
	m_TransferQueueLock(),
	m_GraphicsQueueLock(),
	m_pTransferPool(nullptr),
	m_pGraphicsPool(nullptr),
	m_pTransferBuffers(),
	m_pGraphicsBuffers(),
	m_TransferQueue(VK_NULL_HANDLE),
	m_GraphicsQueue(VK_NULL_HANDLE),
	m_CurrentTransferBuffer(0),
	m_CurrentGraphicsBuffer(0)
{
}

CopyHandlerVK::~CopyHandlerVK()
{
	SAFEDELETE(m_pGraphicsPool);
	SAFEDELETE(m_pTransferPool);

	m_TransferQueue = VK_NULL_HANDLE;
	m_GraphicsQueue = VK_NULL_HANDLE;
	m_pDevice = nullptr;
}

bool CopyHandlerVK::init()
{
	m_pTransferPool = DBG_NEW CommandPoolVK(m_pDevice, m_pDevice->getQueueFamilyIndices().transferFamily.value());
	if (!m_pTransferPool->init())
	{
		return false;
	}

	m_pGraphicsPool = DBG_NEW CommandPoolVK(m_pDevice, m_pDevice->getQueueFamilyIndices().graphicsFamily.value());
	if (!m_pGraphicsPool->init())
	{
		return false;
	}

	for (uint32_t i = 0; i < MAX_COMMAND_BUFFERS; i++)
	{
		m_pTransferBuffers[i] = m_pTransferPool->allocateCommandBuffer();
		if (!m_pTransferBuffers[i])
		{
			return false;
		}

		m_pGraphicsBuffers[i] = m_pGraphicsPool->allocateCommandBuffer();
		if (!m_pGraphicsBuffers[i])
		{
			return false;
		}
	}

	m_TransferQueue = m_pDevice->getTransferQueue();
	m_GraphicsQueue = m_pDevice->getGraphicsQueue();
	return true;
}

void CopyHandlerVK::updateBuffer(BufferVK* pDestination, uint64_t destinationOffset, const void* pSource, uint64_t sizeInBytes)
{
	CommandBufferVK* pCommandBuffer = getNextTransferBuffer();
	pCommandBuffer->reset();
	pCommandBuffer->begin();
	pCommandBuffer->updateBuffer(pDestination, destinationOffset, pSource, sizeInBytes);
	pCommandBuffer->end();

	submitTransferBuffer(pCommandBuffer);
}

void CopyHandlerVK::copyBuffer(BufferVK* pSource, uint64_t sourceOffset, BufferVK* pDestination, uint64_t destinationOffset, uint64_t sizeInBytes)
{
	CommandBufferVK* pCommandBuffer = getNextTransferBuffer();
	pCommandBuffer->reset();
	pCommandBuffer->begin();
	pCommandBuffer->copyBuffer(pSource, sourceOffset, pDestination, destinationOffset, sizeInBytes);
	pCommandBuffer->end();

	submitTransferBuffer(pCommandBuffer);
}

void CopyHandlerVK::updateImage(const void* pPixelData, ImageVK* pImage, uint32_t width, uint32_t height, uint32_t pixelStride, VkImageLayout initalLayout, VkImageLayout finalLayout, uint32_t miplevel)
{
	CommandBufferVK* pCommandBuffer = getNextGraphicsBuffer();
	pCommandBuffer->reset();
	pCommandBuffer->begin();
	
	//Insert barrier if we need to
	if (initalLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		pCommandBuffer->transitionImageLayout(pImage, initalLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, pImage->getMiplevelCount());
	}

	pCommandBuffer->updateImage(pPixelData, pImage, width, height, pixelStride, miplevel);

	//Insert barrier if we need to
	if (finalLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		pCommandBuffer->transitionImageLayout(pImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout, 0, pImage->getMiplevelCount());
	}

	pCommandBuffer->end();

	submitGraphicsBuffer(pCommandBuffer);
}

void CopyHandlerVK::copyBufferToImage(BufferVK* pSource, VkDeviceSize sourceOffset, ImageVK* pImage, uint32_t width, uint32_t height, uint32_t miplevel)
{
	CommandBufferVK* pCommandBuffer = getNextGraphicsBuffer();
	pCommandBuffer->reset();
	pCommandBuffer->begin();
	pCommandBuffer->copyBufferToImage(pSource, sourceOffset, pImage, width, height, miplevel);
	pCommandBuffer->end();

	submitGraphicsBuffer(pCommandBuffer);
}

void CopyHandlerVK::generateMips(ImageVK* pImage)
{
	CommandBufferVK* pCommandBuffer = getNextGraphicsBuffer();
	pCommandBuffer->reset();
	pCommandBuffer->begin();

	const uint32_t miplevelCount = pImage->getMiplevelCount();
	pCommandBuffer->transitionImageLayout(pImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, miplevelCount);
	
	VkExtent2D destinationExtent = {};
	VkExtent2D sourceExtent = { pImage->getExtent().width, pImage->getExtent().height };
	for (uint32_t i = 1; i < miplevelCount; i++)
	{
		destinationExtent = { std::max(sourceExtent.width / 2U, 1u), std::max(sourceExtent.height / 2U, 1U) };
		pCommandBuffer->transitionImageLayout(pImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, i - 1, 1);
		pCommandBuffer->blitImage2D(pImage, i - 1, sourceExtent, pImage, i, destinationExtent);
		sourceExtent = destinationExtent;
	}

	pCommandBuffer->transitionImageLayout(pImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, miplevelCount - 1, 1);
	pCommandBuffer->transitionImageLayout(pImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, miplevelCount);
	pCommandBuffer->end();

	submitGraphicsBuffer(pCommandBuffer);
}

void CopyHandlerVK::waitForResources()
{
	//TODO: Probably should use a fence here? Do we need a lock?
	vkQueueWaitIdle(m_TransferQueue);
	vkQueueWaitIdle(m_GraphicsQueue);
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
	//Need to lock this since commandbuffer submition is not meant to be done from multiple threads? Different sources say different things
	std::scoped_lock<Spinlock> lock(m_TransferQueueLock);
	m_pDevice->executeCommandBuffer(m_TransferQueue, pCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
}

void CopyHandlerVK::submitGraphicsBuffer(CommandBufferVK* pCommandBuffer)
{
	//Need to lock this since commandbuffer submition is not meant to be done from multiple threads? Different sources say different things
	std::scoped_lock<Spinlock> lock(m_GraphicsQueueLock);
	m_pDevice->executeCommandBuffer(m_GraphicsQueue, pCommandBuffer, nullptr, nullptr, 0, nullptr, 0);
}
