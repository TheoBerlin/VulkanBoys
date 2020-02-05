#include "InstanceVK.h"

#include <iostream>
#include <set>
#include "ConsoleHelper.h"

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


InstanceVK::InstanceVK() :
	m_Instance(VK_NULL_HANDLE),
	m_ValidationLayersEnabled(false),
	m_DebugMessenger(VK_NULL_HANDLE)
{
}

InstanceVK::~InstanceVK()
{
	if (m_ValidationLayersEnabled)
	{
		if (m_DebugMessenger != VK_NULL_HANDLE)
		{
			DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
			m_DebugMessenger = VK_NULL_HANDLE;
		}
	}

	if (m_Instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_Instance, nullptr);
		m_Instance = VK_NULL_HANDLE;
		std::cout << "Vulkan Device Destroyed!" << std::endl;
	}
}

bool InstanceVK::finalize(bool validationLayersEnabled)
{
	m_ValidationLayersEnabled = validationLayersEnabled;
	
	if (!initInstance())
		return false;

	if (!initializeDebugMessenger())
		return false;
}

void InstanceVK::release()
{
}

void InstanceVK::addRequiredExtension(const char* extensionName)
{
	m_RequestedRequiredExtensions.push_back(extensionName);
}

void InstanceVK::addOptionalExtension(const char* extensionName)
{
	m_RequestedOptionalExtensions.push_back(extensionName);
}

void InstanceVK::addValidationLayer(const char* layerName)
{
	m_ValidationLayers.push_back(layerName);
}

bool InstanceVK::initInstance()
{
	if (m_ValidationLayersEnabled && !validationLayersSupported())
	{
		std::cerr << "Validation Layers not supported!" << std::endl;
		return false;
	}

	if (!setEnabledExtensions())
	{
		std::cerr << "One or more Required Extensions not supported!" << std::endl;
		return false;
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "Vulkan Application";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_EnabledExtensions.size());
	createInfo.ppEnabledExtensionNames = m_EnabledExtensions.data();

	if (m_ValidationLayersEnabled)
	{
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		populateDebugMessengerCreateInfo(debugCreateInfo);

		createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateInstance(&createInfo, nullptr, &m_Instance), "Failed to create Vulkan Instance!");

	std::cout << "Vulkan Instance created successfully!" << std::endl;
	return true;
}

bool InstanceVK::initializeDebugMessenger()
{
	if (!m_ValidationLayersEnabled)
		return true;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	populateDebugMessengerCreateInfo(createInfo);

	VK_CHECK_RESULT_RETURN_FALSE(CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger), "Failed to set up Debug Messenger!");
	return true;
}

bool InstanceVK::validationLayersSupported()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : m_ValidationLayers)
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

bool InstanceVK::setEnabledExtensions()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(m_RequestedRequiredExtensions.begin(), m_RequestedRequiredExtensions.end());
	std::set<std::string> optionalExtensions(m_RequestedOptionalExtensions.begin(), m_RequestedOptionalExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		if (requiredExtensions.erase(extension.extensionName) > 0)
		{
			m_EnabledExtensions.push_back(extension.extensionName);
		}
		else
		{
			std::cerr << "Required Instance Extension : " << extension.extensionName << " not supported!" << std::endl;
			return false;
		}
		
		if (optionalExtensions.erase(extension.extensionName) > 0)
		{
			m_EnabledExtensions.push_back(extension.extensionName);
		}
		else
		{
			std::cerr << "Optional Instance Extension : " << extension.extensionName << " not supported!" << std::endl;
		}
	}

	return true;
}

void InstanceVK::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr;
}

VkBool32 InstanceVK::DebugCallback(
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