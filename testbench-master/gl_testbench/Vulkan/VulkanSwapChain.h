#pragma once
#ifdef _WIN32
	#define VK_USE_PLATFORM_WIN32_KHR
	#include <vulkan/vulkan.h>
#else
	#error Invalid platform, only Windows is supported
#endif

#include "Common.h"

#include <vector>
#include <stdint.h>

class VulkanDevice;

class VulkanSwapChain
{
public:
	VulkanSwapChain(VulkanDevice* pDevice, VkFormat requestedFormat, uint32_t imageCount, bool verticalSync);
	~VulkanSwapChain();

	DECL_NO_COPY(VulkanSwapChain);

	VkResult aquireNextImage(VkSemaphore imageSemaphore);
	VkResult present(VkSemaphore renderSemaphore);
	void resize(uint32_t width, uint32_t height);

	VkFramebuffer getCurrentFramebuffer() { return m_Framebuffers[m_ImageIndex]; }
	uint32_t getImageIndex() const { return m_ImageIndex; }
	VkFormat getFormat() const { return m_Format.format; }
	VkExtent2D getExtent() const { return m_Extent; }
private:
	void createSurface();
	void selectPresentationMode(bool verticalSync);
	void selectFormat(VkFormat requestedFormat);
	void createSwapChain(uint32_t width, uint32_t height);
	void createImageViews();
	void createDepthStencil();
	void createFramebuffers();
	void release();
private:
	std::vector<VkImage> m_Images;
	std::vector<VkImageView> m_ImageViews;
	std::vector<VkFramebuffer> m_Framebuffers;
	VulkanDevice* m_pDevice;
	VkSurfaceKHR m_Surface;
	VkSwapchainKHR m_SwapChain;
	VkExtent2D m_Extent;
	VkSurfaceFormatKHR m_Format;
	VkPresentModeKHR m_PresentationMode;
	VkImage m_DepthStencilImage;
	VkImageView m_DepthStencilImageView;
	uint32_t m_ImageIndex;
	uint32_t m_ImageCount;
};

