#pragma once
#include "Core/Spinlock.h"

#include "VulkanCommon.h"

class ImageVK;
class DeviceVK;
class BufferVK;
class CommandPoolVK;
class CommandBufferVK;

#define MAX_COMMAND_BUFFERS 3

class CopyHandlerVK
{
public:
	CopyHandlerVK(DeviceVK* pDevice);
	~CopyHandlerVK();

	bool init();

	void updateBuffer(BufferVK* pDestination, uint64_t destinationOffset, const void* pSource, uint64_t sizeInBytes);
	void copyBuffer(BufferVK* pSource, uint64_t sourceOffset, BufferVK* pDestination, uint64_t destinationOffset, uint64_t sizeInBytes);

	void updateImage(const void* pPixelData, ImageVK* pImage, uint32_t width, uint32_t height, VkImageLayout initalLayout, VkImageLayout finalLayout);
	void copyBufferToImage(BufferVK* pSource, VkDeviceSize sourceOffset, ImageVK* pImage, uint32_t width, uint32_t height);

	void waitForResources();

private:
	CommandBufferVK* getNextTransferBuffer();
	CommandBufferVK* getNextGraphicsBuffer();
	void submitTransferBuffer(CommandBufferVK* pCommandBuffer);
	void submitGraphicsBuffer(CommandBufferVK* pCommandBuffer);

private:
	//Should proably be controlled by the device or have a queue wrapper that controlls these
	Spinlock m_TransferQueueLock;
	Spinlock m_GraphicsQueueLock;
	DeviceVK* m_pDevice;
	CommandPoolVK* m_pTransferPool;
	CommandPoolVK* m_pGraphicsPool;
	CommandBufferVK* m_pTransferBuffers[MAX_COMMAND_BUFFERS];
	CommandBufferVK* m_pGraphicsBuffers[MAX_COMMAND_BUFFERS];
	VkQueue m_TransferQueue;
	VkQueue m_GraphicsQueue;
	uint32_t m_CurrentTransferBuffer;
	uint32_t m_CurrentGraphicsBuffer;
};