#include "VulkanDevice.h"
#include <set>

const std::vector<const char*> VulkanDevice::s_ValidationLayers = 
{
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> VulkanDevice::s_RequiredInstanceExtensions =
{
#if VALIDATION_LAYERS_ENABLED
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
	VK_KHR_SURFACE_EXTENSION_NAME,
	"VK_KHR_win32_surface"
};

const std::vector<const char*> VulkanDevice::s_RequiredDeviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

VulkanDevice::VulkanDevice() 
{
}

VulkanDevice::~VulkanDevice()
{
	release();
}

void VulkanDevice::initialize(const char applicationName[])
{
	initializeInstance(applicationName);
	createDebugMessenger();
	initializePhysicalDevice();
	initializeLogicalDevice();
}

void VulkanDevice::release()
{
	if (m_Device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_Device, nullptr);
		m_Device = VK_NULL_HANDLE;
	}

	if (VALIDATION_LAYERS_ENABLED)
	{
		if (m_DebugMessenger != VK_NULL_HANDLE)
		{
			DestroyDebugUtilsMessengerEXT(m_VKInstance, m_DebugMessenger, nullptr);
			m_DebugMessenger = VK_NULL_HANDLE;
		}
	}

	if (m_VKInstance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_VKInstance, nullptr);
		m_VKInstance = VK_NULL_HANDLE;
	}

	std::cout << "Vulkan Device Destroyed!" << std::endl;
}

void VulkanDevice::initializeInstance(const char applicationName[])
{
	listSupportedInstanceExtensions();

	if (VALIDATION_LAYERS_ENABLED && !validationLayersSupported())
		throw std::runtime_error("Validation Layers not supported!");

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = applicationName;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(s_RequiredInstanceExtensions.size());
	createInfo.ppEnabledExtensionNames = s_RequiredInstanceExtensions.data();

	if (VALIDATION_LAYERS_ENABLED)
	{
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		populateDebugMessengerCreateInfo(debugCreateInfo);

		createInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_VKInstance) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Vulkan Instance!");

	std::cout << "Vulkan Instance created successfully!" << std::endl;
	
}

void VulkanDevice::initializePhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_VKInstance, &deviceCount, nullptr);

	if (deviceCount == 0)
		throw std::runtime_error("Failed to find GPUs with Vulkan Support!");

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(m_VKInstance, &deviceCount, physicalDevices.data());

	std::multimap<int32_t, VkPhysicalDevice> physicalDeviceCandidates;

	for (const auto& physicalDevice : physicalDevices) 
	{
		int32_t score = rateDevice(physicalDevice);
		physicalDeviceCandidates.insert(std::make_pair(score, physicalDevice));
	}

	// Check if the best candidate is suitable at all
	if (physicalDeviceCandidates.rbegin()->first <= 0)
		throw std::runtime_error("Failed to find a suitable GPU!");
	
	
	m_PhysicalDevice = physicalDeviceCandidates.rbegin()->second;
	m_DeviceQueueFamilyIndices = findQueueFamilies(m_PhysicalDevice);
}

void VulkanDevice::initializeLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { m_DeviceQueueFamilyIndices.graphicsFamily.value(), m_DeviceQueueFamilyIndices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(s_RequiredDeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = s_RequiredDeviceExtensions.data();

	if (VALIDATION_LAYERS_ENABLED)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(m_Device, m_DeviceQueueFamilyIndices.graphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, m_DeviceQueueFamilyIndices.presentFamily.value(), 0, &m_PresentQueue);
}

void VulkanDevice::createDebugMessenger()
{
	if (!VALIDATION_LAYERS_ENABLED)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_VKInstance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to set up Debug Messenger!");
	}
}

void VulkanDevice::listSupportedInstanceExtensions()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::cout << "Supported Vulkan Extensions:" << std::endl;

	for (const auto& extension : extensions) 
		std::cout << "  - " << extension.extensionName << std::endl;

	std::cout << std::endl;
}

bool VulkanDevice::validationLayersSupported()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : s_ValidationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

int32_t VulkanDevice::rateDevice(const VkPhysicalDevice& physicalDevice)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	if (!indices.IsComplete() || !extensionsSupported)
		return 0;

	int score = 1;

	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 1000;

	return score;
}

bool VulkanDevice::checkDeviceExtensionSupport(const VkPhysicalDevice& device)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(s_RequiredDeviceExtensions.begin(), s_RequiredDeviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

QueueFamilyIndices VulkanDevice::findQueueFamilies(const VkPhysicalDevice& physicalDevice)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
			indices.presentFamily = i; //Assume present support at this stage
		}
		
		if (indices.IsComplete())
			break;

		i++;
	}

	return indices;
}

void VulkanDevice::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr;
}

VkBool32 VulkanDevice::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "---Validation Layer: ";

	if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) >= 1)
	{
		std::cerr << pCallbackData->pMessage << std::endl;
	}
	else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) >= 1)
	{
		std::cerr << greenText << pCallbackData->pMessage << whiteText << std::endl;
	}
	else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) >= 1)
	{
		std::cerr << yellowText << pCallbackData->pMessage << whiteText << std::endl;
	}
	else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) >= 1)
	{
		std::cerr << redText << pCallbackData->pMessage << whiteText << std::endl;
	}


	return VK_FALSE;
}
