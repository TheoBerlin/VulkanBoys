#pragma once
#include "Core/Spinlock.h"

#include "VulkanCommon.h"

class ImageVK;
class DeviceVK;
class BufferVK;
class InstanceVK;
class CommandPoolVK;
class CommandBufferVK;

#define MAX_COMMAND_BUFFERS 6

class CopyHandlerVK
{
public:
	CopyHandlerVK(DeviceVK* pDevice, InstanceVK* pInstance);
	~CopyHandlerVK();

	bool init();

	void updateBuffer(BufferVK* pDestination, uint64_t destinationOffset, const void* pSource, uint64_t sizeInBytes);
	void copyBuffer(BufferVK* pSource, uint64_t sourceOffset, BufferVK* pDestination, uint64_t destinationOffset, uint64_t sizeInBytes);

	void updateImage(const void* pPixelData, ImageVK* pImage, uint32_t width, uint32_t height, uint32_t pixelStride, VkImageLayout initalLayout, VkImageLayout finalLayout, uint32_t miplevel, uint32_t layer);
	void copyBufferToImage(BufferVK* pSource, VkDeviceSize sourceOffset, ImageVK* pImage, uint32_t width, uint32_t height, uint32_t miplevel, uint32_t layer);

	void generateMips(ImageVK* pImage);

	void waitForResources();

private:
	CommandBufferVK* getNextTransferBuffer();
	CommandBufferVK* getNextGraphicsBuffer();
	void submitTransferBuffer(CommandBufferVK* pCommandBuffer);
	void submitGraphicsBuffer(CommandBufferVK* pCommandBuffer);

private:
	DeviceVK* m_pDevice;
	InstanceVK* m_pInstance;
	CommandPoolVK* m_pTransferPool;
	CommandPoolVK* m_pGraphicsPool;
	CommandBufferVK* m_pTransferBuffers[MAX_COMMAND_BUFFERS];
	CommandBufferVK* m_pGraphicsBuffers[MAX_COMMAND_BUFFERS];
	uint32_t m_CurrentTransferBuffer;
	uint32_t m_CurrentGraphicsBuffer;
};