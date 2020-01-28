#include "VulkanSwapChain.h"
#include "VulkanDevice.h"

#include <iostream>
#include <numeric>
#include <algorithm>
#include <SDL.h>
#include <SDL_syswm.h>

#ifdef min
	#undef min
#endif

#ifdef max
	#undef max
#endif

VulkanSwapChain::VulkanSwapChain()
	: m_pDevice(nullptr),
	m_SwapChain(VK_NULL_HANDLE),
	m_Surface(VK_NULL_HANDLE),
	m_DepthStencilImage(VK_NULL_HANDLE),
	m_DepthStencilImageView(VK_NULL_HANDLE),
	m_Renderpass(VK_NULL_HANDLE),
	m_ImageMemory(VK_NULL_HANDLE),
	m_Extent(),
	m_Images(),
	m_ImageViews(),
	m_Framebuffers(),
	m_ImageIndex(UINT32_MAX),
	m_ImageCount(0)
{
}

VulkanSwapChain::~VulkanSwapChain()
{
}

void VulkanSwapChain::initialize(VulkanDevice* pDevice, SDL_Window* pWindow, VkRenderPass renderpass, VkFormat requestedFormat, uint32_t imageCount, bool verticalSync)
{
	m_pDevice = pDevice;
	m_ImageCount = imageCount;
	m_Renderpass = renderpass;

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(pWindow, &wmInfo);
	createSurface(wmInfo.info.win.window);

	//Check for presentationsupport
	VkBool32 presentSupport = false;
	QueueFamilyIndices familyIndices = m_pDevice->getQueueFamilyIndices();
	vkGetPhysicalDeviceSurfaceSupportKHR(m_pDevice->getPhysicalDevice(), familyIndices.presentFamily.value(), m_Surface, &presentSupport);
	if (presentSupport)
	{
		int32_t width = 0;
		int32_t height = 0;
		SDL_GetWindowSize(pWindow, &width, &height);

		selectFormat(requestedFormat);
		selectPresentationMode(verticalSync);

		createSwapChain(width, height);
		createImageViews();
		createDepthStencil();
		createFramebuffers();
	}
	else
	{
		std::cerr << "Presentation is not supported by the selected physicaldevice" << std::endl;
	}
}

void VulkanSwapChain::release()
{
	releaseResources();

	if (m_Surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_pDevice->getInstance(), m_Surface, nullptr);
		m_Surface = VK_NULL_HANDLE;
	}
}

VkResult VulkanSwapChain::acquireNextImage(VkSemaphore imageSemaphore)
{
	VkResult result = vkAcquireNextImageKHR(m_pDevice->getDevice(), m_SwapChain, UINT64_MAX, imageSemaphore, VK_NULL_HANDLE, &m_ImageIndex);
	return VK_SUCCESS;
}

VkResult VulkanSwapChain::present(VkSemaphore renderSemaphore)
{
	VkSemaphore waitSemaphores[] = { renderSemaphore };

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.swapchainCount		= 1;
	presentInfo.pSwapchains			= &m_SwapChain;
	presentInfo.waitSemaphoreCount	= 1;
	presentInfo.pWaitSemaphores		= waitSemaphores;
	presentInfo.pResults			= nullptr;
	presentInfo.pImageIndices		= &m_ImageIndex;

	VkResult result = vkQueuePresentKHR(m_pDevice->getPresentQueue(), &presentInfo);
	return VK_SUCCESS;
}

void VulkanSwapChain::resize(uint32_t width, uint32_t height)
{
	//Handle minimize
	if (width != 0 && height != 0)
	{
		vkDeviceWaitIdle(m_pDevice->getDevice());
	
		release();

		createSwapChain(width, height);
		createImageViews();
		createDepthStencil();
		createFramebuffers();
	}
}

void VulkanSwapChain::createSurface(HWND hwnd)
{
	VkWin32SurfaceCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	info.pNext = nullptr;
	info.flags = 0;
	info.hwnd = hwnd;
	info.hinstance = (HINSTANCE)GetModuleHandle(nullptr);
	
	VkResult result = vkCreateWin32SurfaceKHR(m_pDevice->getInstance(), &info, nullptr, &m_Surface);
	if (result != VK_SUCCESS)
	{
		m_Surface = VK_NULL_HANDLE;
		std::cerr << "vkCreateWin32SurfaceKHR failed" << std::endl;
	}
	else
	{
		std::cout << "Created surface" << std::endl;
	}
}

void VulkanSwapChain::selectPresentationMode(bool verticalSync)
{
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_pDevice->getPhysicalDevice(), m_Surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_pDevice->getPhysicalDevice(), m_Surface, &presentModeCount, presentModes.data());

	m_PresentationMode = VK_PRESENT_MODE_FIFO_KHR;
	if (!verticalSync)
	{
		//Search for the mailbox mode
		for (VkPresentModeKHR& availablePresentMode : presentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				m_PresentationMode = availablePresentMode;
				break;
			}
		}

		//If mailbox is not available we choose immediete
		if (m_PresentationMode == VK_PRESENT_MODE_FIFO_KHR)
		{
			for (VkPresentModeKHR& availablePresentMode : presentModes)
			{
				if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					m_PresentationMode = availablePresentMode;
					break;
				}
			}
		}
	}
}

void VulkanSwapChain::selectFormat(VkFormat requestedFormat)
{
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_pDevice->getPhysicalDevice(), m_Surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_pDevice->getPhysicalDevice(), m_Surface, &formatCount, formats.data());

	m_Format = { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	for (VkSurfaceFormatKHR& availableFormat : formats)
	{
		if (availableFormat.format == requestedFormat && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			m_Format = availableFormat;
			break;
		}
	}

	if (m_Format.format == VK_FORMAT_UNDEFINED)
	{
		std::cerr << "Format is not supported on selected PhysicalDevice" << std::endl;
	}
}

void VulkanSwapChain::createSwapChain(uint32_t width, uint32_t height)
{
	VkSurfaceCapabilitiesKHR capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_pDevice->getPhysicalDevice(), m_Surface, &capabilities);

	VkExtent2D newExtent = {};
	if (capabilities.currentExtent.width	!= std::numeric_limits<uint32_t>::max() ||
		capabilities.currentExtent.height	!= std::numeric_limits<uint32_t>::max())
	{
		newExtent = capabilities.currentExtent;
	}
	else
	{
		newExtent.width		= std::max(capabilities.minImageExtent.width,	std::min(capabilities.maxImageExtent.width,		width));
		newExtent.height	= std::max(capabilities.minImageExtent.height,	std::min(capabilities.maxImageExtent.height,	height));
	}

	newExtent.width		= std::max(newExtent.width, 1u);
	newExtent.height	= std::max(newExtent.height, 1u);
	m_Extent = newExtent;

	std::cout << "Chosen SwapChain extent w: " << m_Extent.width << " h: " << newExtent.height << std::endl;

	VkSwapchainCreateInfoKHR swapChainInfo = {};
	swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainInfo.pNext = nullptr;
	swapChainInfo.surface			= m_Surface;
	swapChainInfo.minImageCount		= m_ImageCount;
	swapChainInfo.imageFormat		= m_Format.format;
	swapChainInfo.imageColorSpace	= m_Format.colorSpace;
	swapChainInfo.imageExtent		= m_Extent;
	swapChainInfo.imageArrayLayers	= 1;
	swapChainInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; //Use as color attachment and clear
	swapChainInfo.preTransform		= capabilities.currentTransform;
	swapChainInfo.compositeAlpha	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainInfo.presentMode		= m_PresentationMode;
	swapChainInfo.clipped			= VK_TRUE;
	swapChainInfo.oldSwapchain		= VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(m_pDevice->getDevice(), &swapChainInfo, nullptr, &m_SwapChain);
	if (result != VK_SUCCESS)
	{
		std::cerr << "vkCreateSwapchainKHR failed" << std::endl;
	}
	else
	{
		std::cout << "Created SwapChain" << std::endl;
	}
}

void VulkanSwapChain::createImageViews()
{
	//Get SwapChain images
	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(m_pDevice->getDevice(), m_SwapChain, &imageCount, nullptr);
	if (imageCount != m_ImageCount)
		std::cerr << "Requsted imageCount (=" << m_ImageCount << ") not avaiilable. Number of images in SwapChain=" << imageCount << std::endl;

	m_ImageCount = imageCount;

	m_Images.resize(m_ImageCount);
	vkGetSwapchainImagesKHR(m_pDevice->getDevice(), m_SwapChain, &imageCount, m_Images.data());

	m_ImageViews.resize(m_ImageCount);
	VkImageViewCreateInfo imageViewInfo = {};
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.pNext = nullptr;
	imageViewInfo.flags = 0;
	imageViewInfo.format = m_Format.format;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewInfo.subresourceRange.layerCount = 1;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;
	imageViewInfo.subresourceRange.levelCount = 1;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

	VkResult result = VK_SUCCESS;
	for (uint32_t i = 0; i < imageCount; i++)
	{
		imageViewInfo.image = m_Images[i];
		result = vkCreateImageView(m_pDevice->getDevice(), &imageViewInfo, nullptr, &m_ImageViews[i]);
		if (result != VK_SUCCESS)
		{
			std::cerr << "vkCreateImageView failed" << std::endl;
			break;
		}
	}
}

void VulkanSwapChain::createDepthStencil()
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.pNext = nullptr;
	imageInfo.flags = 0;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.pQueueFamilyIndices = nullptr;
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.extent = { m_Extent.width, m_Extent.height, 1 };

	VkResult result = vkCreateImage(m_pDevice->getDevice(), &imageInfo, nullptr, &m_DepthStencilImage);
	if (result != VK_SUCCESS)
	{
		std::cerr << "vkCreateImage failed" << std::endl;
		return;
	}
	else
	{
		std::cout << "Created DepthStencil-Image" << std::endl;
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_pDevice->getDevice(), m_DepthStencilImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(m_pDevice->getPhysicalDevice(), memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(m_pDevice->getDevice(), &allocInfo, nullptr, &m_ImageMemory) != VK_SUCCESS) 
	{
		std::cerr << "failed to allocate image memory" << std::endl;
		return;
	}

	vkBindImageMemory(m_pDevice->getDevice(), m_DepthStencilImage, m_ImageMemory, 0);

	VkImageViewCreateInfo imageViewInfo = {};
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.pNext = nullptr;
	imageViewInfo.flags = 0;
	imageViewInfo.format	= imageInfo.format;
	imageViewInfo.image		= m_DepthStencilImage;
	imageViewInfo.viewType	= VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	imageViewInfo.subresourceRange.layerCount		= 1;
	imageViewInfo.subresourceRange.baseArrayLayer	= 0;
	imageViewInfo.subresourceRange.levelCount		= 1;
	imageViewInfo.subresourceRange.baseMipLevel		= 0;
	imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

	result = vkCreateImageView(m_pDevice->getDevice(), &imageViewInfo, nullptr, &m_DepthStencilImageView);
	if (result != VK_SUCCESS)
	{
		std::cerr << "vkCreateImageView failed" << std::endl;
		return;
	}
	else
	{
		std::cout << "Created DepthStencil-ImageView" << std::endl;
	}
}

void VulkanSwapChain::createFramebuffers()
{
	m_Framebuffers.resize(m_ImageCount);

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.pNext = nullptr;
	framebufferInfo.flags = 0;
	framebufferInfo.width = m_Extent.width;
	framebufferInfo.height = m_Extent.height;
	framebufferInfo.attachmentCount = 2;
	framebufferInfo.renderPass = m_Renderpass;
	framebufferInfo.layers = 1;

	VkResult result = VK_SUCCESS;
	for (uint32_t i = 0; i < m_ImageCount; i++)
	{
		VkImageView attachments[] = { m_ImageViews[i], m_DepthStencilImageView };
		framebufferInfo.pAttachments = attachments;

		result = vkCreateFramebuffer(m_pDevice->getDevice(), &framebufferInfo, nullptr, &m_Framebuffers[i]);
		if (result != VK_SUCCESS)
		{
			std::cerr << "vkCreateFramebuffer failed" << std::endl;
			break;
		}
		else
		{
			std::cout << "Created framebuffer" << std::endl;
		}
	}
}

void VulkanSwapChain::releaseResources()
{
	vkDeviceWaitIdle(m_pDevice->getDevice());

	if (m_SwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_pDevice->getDevice(), m_SwapChain, nullptr);
		m_SwapChain = VK_NULL_HANDLE;
	}

	if (m_DepthStencilImage != VK_NULL_HANDLE)
	{
		vkDestroyImage(m_pDevice->getDevice(), m_DepthStencilImage, nullptr);
		m_DepthStencilImage = VK_NULL_HANDLE;
	}

	if (m_DepthStencilImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_pDevice->getDevice(), m_DepthStencilImageView, nullptr);
		m_DepthStencilImageView = VK_NULL_HANDLE;
	}

	if (m_ImageMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(m_pDevice->getDevice(), m_ImageMemory, nullptr);
		m_ImageMemory = VK_NULL_HANDLE;
	}

	for (VkImageView& imageView : m_ImageViews)
	{
		if (imageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_pDevice->getDevice(), imageView, nullptr);
			imageView = VK_NULL_HANDLE;
		}
	}

	for (VkFramebuffer& framebuffer: m_Framebuffers)
	{
		if (framebuffer != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(m_pDevice->getDevice(), framebuffer, nullptr);
			framebuffer = VK_NULL_HANDLE;
		}
	}
}
