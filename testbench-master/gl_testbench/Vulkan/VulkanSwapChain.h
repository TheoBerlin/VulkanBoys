#pragma once
#include "Common.h"

#include <vector>
#include <stdint.h>

class VulkanDevice;
struct SDL_Window;

class VulkanSwapChain
{
public:
	VulkanSwapChain();
	~VulkanSwapChain();

	DECL_NO_COPY(VulkanSwapChain);

	void initialize(VulkanDevice* pDevice, SDL_Window* pWindow, VkRenderPass renderpass, VkFormat requestedFormat, uint32_t imageCount, bool verticalSync);
	void release();

	VkResult acquireNextImage(VkSemaphore imageSemaphore);
	VkResult present(VkSemaphore renderSemaphore);
	void resize(uint32_t width, uint32_t height);

	VkFramebuffer getCurrentFramebuffer() { return m_Framebuffers[m_ImageIndex]; }
	uint32_t getImageIndex() const { return m_ImageIndex; }
	VkFormat getFormat() const { return m_Format.format; }
	VkExtent2D getExtent() const { return m_Extent; }
private:
	void createSurface(HWND hwnd);
	void selectPresentationMode(bool verticalSync);
	void selectFormat(VkFormat requestedFormat);
	void createSwapChain(uint32_t width, uint32_t height);
	void createImageViews();
	void createDepthStencil();
	void createFramebuffers();
	void releaseResources();
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
	VkDeviceMemory m_ImageMemory;
	VkImageView m_DepthStencilImageView;
	VkRenderPass m_Renderpass;
	uint32_t m_ImageIndex;
	uint32_t m_ImageCount;
};

