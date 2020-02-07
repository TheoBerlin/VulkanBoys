#pragma once
#include "VulkanCommon.h"

#include <vector>

class IWindow;
class ImageVK;
class DeviceVK;
class InstanceVK;
class ImageViewVK;

class SwapChainVK
{
public:
	SwapChainVK(InstanceVK* pInstance, DeviceVK* pDevice);
	~SwapChainVK();

	DECL_NO_COPY(SwapChainVK);

	void init(IWindow* pWindow, VkFormat requestedFormat, uint32_t imageCount, bool verticalSync);
	void release();

	VkResult acquireNextImage(VkSemaphore imageSemaphore);
	VkResult present(VkSemaphore renderSemaphore);
	void resize(uint32_t width, uint32_t height);

	ImageVK* getImage(uint32_t index) const { return m_Images[index]; }
	ImageViewVK* getImageView(uint32_t index) const { return m_ImageViews[index]; }
	ImageVK* getDepthStencil() const { return m_pDepthStencilImage; }
	ImageViewVK* getDepthStencilView() const { return m_pDepthStencilImageView; }
	uint32_t getImageIndex() const { return m_ImageIndex; }
	VkFormat getFormat() const { return m_Format.format; }
	VkExtent2D getExtent() const { return m_Extent; }

private:
	void createSurface(IWindow* pWindow);
	void selectPresentationMode(bool verticalSync);
	void selectFormat(VkFormat requestedFormat);
	void createSwapChain(uint32_t width, uint32_t height);
	void createImageViews();
	void createDepthStencil();
	void releaseResources();

private:
	std::vector<ImageVK*> m_Images;
	std::vector<ImageViewVK*> m_ImageViews;
	DeviceVK* m_pDevice;
	InstanceVK* m_pInstance;
	ImageVK* m_pDepthStencilImage;
	ImageViewVK* m_pDepthStencilImageView;
	VkSurfaceKHR m_Surface;
	VkSwapchainKHR m_SwapChain;

	VkExtent2D m_Extent;
	VkSurfaceFormatKHR m_Format;
	VkPresentModeKHR m_PresentationMode;
	uint32_t m_ImageIndex;
	uint32_t m_ImageCount;
};

