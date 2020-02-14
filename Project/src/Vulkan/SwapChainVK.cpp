#include "SwapChainVK.h"
#include "DeviceVK.h"
#include "InstanceVK.h"
#include "ImageVK.h"
#include "ImageViewVK.h"

#include "Core/GLFWWindow.h"

#include <algorithm>

#ifdef max
	#undef max
#endif

#ifdef min
	#undef min
#endif

SwapChainVK::SwapChainVK(InstanceVK* pInstance, DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_pInstance(pInstance),
	m_pDepthStencilImage(nullptr),
	m_pDepthStencilImageView(nullptr),
	m_SwapChain(VK_NULL_HANDLE),
	m_Surface(VK_NULL_HANDLE),
	m_Extent(),
	m_Images(),
	m_ImageViews(),
	m_ImageIndex(UINT32_MAX),
	m_ImageCount(0)
{
}

SwapChainVK::~SwapChainVK()
{
	release();
}

void SwapChainVK::init(IWindow* pWindow, VkFormat requestedFormat, uint32_t imageCount, bool verticalSync)
{
	m_ImageCount = imageCount;

	createSurface(pWindow);

	//Check for presentationsupport
	VkBool32 presentSupport = false;
	QueueFamilyIndices familyIndices = m_pDevice->getQueueFamilyIndices();
	vkGetPhysicalDeviceSurfaceSupportKHR(m_pDevice->getPhysicalDevice(), familyIndices.presentFamily.value(), m_Surface, &presentSupport);
	if (presentSupport)
	{
		selectFormat(requestedFormat);
		selectPresentationMode(verticalSync);

		int32_t width	= pWindow->getWidth();
		int32_t height	= pWindow->getHeight();
		createSwapChain(width, height);

		createImageViews();
		createDepthStencil();
	}
	else
	{
		LOG("--- SwapChain: Presentation is not supported by the selected physicaldevice");
	}
}

void SwapChainVK::release()
{
	releaseResources();

	if (m_Surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(m_pInstance->getInstance(), m_Surface, nullptr);
		m_Surface = VK_NULL_HANDLE;
	}
}

VkResult SwapChainVK::acquireNextImage(VkSemaphore imageSemaphore)
{
	VkResult result = vkAcquireNextImageKHR(m_pDevice->getDevice(), m_SwapChain, UINT64_MAX, imageSemaphore, VK_NULL_HANDLE, &m_ImageIndex);
	return VK_SUCCESS;
}

VkResult SwapChainVK::present(VkSemaphore renderSemaphore)
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

void SwapChainVK::resize(uint32_t width, uint32_t height)
{
	//Handle minimize
	if (width != 0 && height != 0)
	{
		m_pDevice->wait();

		releaseResources();

		createSwapChain(width, height);
		createImageViews();
		createDepthStencil();
	}
}

void SwapChainVK::createSurface(IWindow* pWindow)
{
	GLFWwindow* pNativeWindow = reinterpret_cast<GLFWwindow*>(pWindow->getNativeHandle());

	VK_CHECK_RESULT(glfwCreateWindowSurface(m_pInstance->getInstance(), pNativeWindow, nullptr, &m_Surface), "Failed to create Surface");
	D_LOG("--- SwapChain: Vulkan SwapChain created successfully");
}

void SwapChainVK::selectPresentationMode(bool verticalSync)
{
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_pDevice->getPhysicalDevice(), m_Surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_pDevice->getPhysicalDevice(), m_Surface, &presentModeCount, presentModes.data());

	m_PresentationMode = VK_PRESENT_MODE_FIFO_KHR;
	if (verticalSync)
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
	}
	else
	{
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

	D_LOG("--- SwapChain: Selected PresentationMode=%s", presentatModeAsString(m_PresentationMode));
}

void SwapChainVK::selectFormat(VkFormat requestedFormat)
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
		LOG("Format '%s' is not supported on selected PhysicalDevice", formatAsString(requestedFormat));
	}
}

void SwapChainVK::createSwapChain(uint32_t width, uint32_t height)
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
		newExtent.width		= std::max(capabilities.minImageExtent.width,	 std::min(capabilities.maxImageExtent.width, width));
		newExtent.height	= std::max(capabilities.minImageExtent.height,	std::min(capabilities.maxImageExtent.height, height));
	}

	newExtent.width		= std::max(newExtent.width, 1u);
	newExtent.height	= std::max(newExtent.height, 1u);
	m_Extent = newExtent;

	D_LOG("Chosen SwapChain extent w=%d h=%d", m_Extent.width, m_Extent.height);

	VkSwapchainCreateInfoKHR swapChainInfo = {};
	swapChainInfo.sType				= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainInfo.pNext				= nullptr;
	swapChainInfo.surface			= m_Surface;
	swapChainInfo.minImageCount		= m_ImageCount;
	swapChainInfo.imageFormat		= m_Format.format;
	swapChainInfo.imageColorSpace	= m_Format.colorSpace;
	swapChainInfo.imageExtent		= m_Extent;
	swapChainInfo.imageArrayLayers	= 1;
	swapChainInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapChainInfo.preTransform		= capabilities.currentTransform;
	swapChainInfo.compositeAlpha	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainInfo.presentMode		= m_PresentationMode;
	swapChainInfo.clipped			= VK_TRUE;
	swapChainInfo.oldSwapchain		= VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateSwapchainKHR(m_pDevice->getDevice(), &swapChainInfo, nullptr, &m_SwapChain), "vkCreateSwapchainKHR failed");
	D_LOG("--- SwapChain: Vulkan SwapChain created successfully");
}

void SwapChainVK::createImageViews()
{
	//Get SwapChain images
	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(m_pDevice->getDevice(), m_SwapChain, &imageCount, nullptr);
	if (imageCount != m_ImageCount)
	{
		LOG("Requsted imageCount (=%d) not available. ImageCount=%d", m_ImageCount, imageCount);
	}

	m_ImageCount = imageCount;

	std::vector<VkImage> images(m_ImageCount);
	vkGetSwapchainImagesKHR(m_pDevice->getDevice(), m_SwapChain, &imageCount, images.data());

	//Create image and imageview objects
	ImageViewParams imageViewParams = {};
	imageViewParams.Type = VK_IMAGE_VIEW_TYPE_2D;
	imageViewParams.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewParams.LayerCount = 1;
	imageViewParams.FirstLayer = 0;
	imageViewParams.MipLevels = 1;
	imageViewParams.FirstMipLevel = 0;

	for (VkImage& image : images)
	{
		ImageVK* pImage = DBG_NEW ImageVK(image, m_Format.format);
		m_Images.emplace_back(pImage);

		ImageViewVK* pImageView = DBG_NEW ImageViewVK(m_pDevice, pImage);
		pImageView->init(imageViewParams);
		m_ImageViews.emplace_back(pImageView);
	}
}

void SwapChainVK::createDepthStencil()
{
	ImageParams depthStencilParams = {};
	depthStencilParams.Type				= VK_IMAGE_TYPE_2D;
	depthStencilParams.Extent			= { m_Extent.width, m_Extent.height, 1 };
	depthStencilParams.MipLevels		= 1;
	depthStencilParams.ArrayLayers		= 1;
	depthStencilParams.Format			= VK_FORMAT_D24_UNORM_S8_UINT;
	depthStencilParams.MemoryProperty	= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	depthStencilParams.Samples			= VK_SAMPLE_COUNT_1_BIT;
	depthStencilParams.Usage			= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	m_pDepthStencilImage = DBG_NEW ImageVK(m_pDevice);
	m_pDepthStencilImage->init(depthStencilParams);

	ImageViewParams depthStencilViewParams = {};
	depthStencilViewParams.Type				= VK_IMAGE_VIEW_TYPE_2D;
	depthStencilViewParams.FirstLayer		= 0;
	depthStencilViewParams.LayerCount		= 1;
	depthStencilViewParams.MipLevels		= 1;
	depthStencilViewParams.FirstMipLevel	= 0;
	depthStencilViewParams.AspectFlags		= VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	
	m_pDepthStencilImageView = DBG_NEW ImageViewVK(m_pDevice, m_pDepthStencilImage);
	m_pDepthStencilImageView->init(depthStencilViewParams);
}

void SwapChainVK::releaseResources()
{
	m_pDevice->wait();
	if (m_SwapChain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_pDevice->getDevice(), m_SwapChain, nullptr);
		m_SwapChain = VK_NULL_HANDLE;
	}

	for (ImageVK* image : m_Images)
	{
		SAFEDELETE(image);
	}
	m_Images.clear();

	for (ImageViewVK* imageView : m_ImageViews)
	{
		SAFEDELETE(imageView);
	}
	m_ImageViews.clear();

	SAFEDELETE(m_pDepthStencilImage);
	SAFEDELETE(m_pDepthStencilImageView);
}
