#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>
#include <iostream>
#include <map>

#include "../ConsoleHelper.h"
#include <optional>

#ifdef NDEBUG
#define VALIDATION_LAYERS_ENABLED false
#else
#define VALIDATION_LAYERS_ENABLED true
#endif

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

class VulkanDevice
{
public:
	VulkanDevice();
	~VulkanDevice();

	void initialize(const char applicationName[]);
	void release();

	const VkInstance& getInstance() { return m_VKInstance; }
	const VkPhysicalDevice getPhysicalDevice() { return m_PhysicalDevice; }
	const VkDevice getDevice() { return m_Device; }
	const QueueFamilyIndices& getQueueFamilyIndices() { return m_DeviceQueueFamilyIndices; }

	const VkQueue& getGraphicsQueue() { return m_GraphicsQueue; }
	const VkQueue& getPresentQueue() { return m_PresentQueue; }

private:
	void initializeInstance(const char applicationName[]);
	void initializePhysicalDevice();
	void initializeLogicalDevice();
	void createDebugMessenger();
	
	void listSupportedInstanceExtensions();
	bool validationLayersSupported();

	int32_t rateDevice(const VkPhysicalDevice& physicalDevice);
	bool checkDeviceExtensionSupport(const VkPhysicalDevice& physicalDevice);
	QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physicalDevice);
	
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

private:
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
	
private:
	VkInstance m_VKInstance;
	
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_Device;
	
	QueueFamilyIndices m_DeviceQueueFamilyIndices;

	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

	VkDebugUtilsMessengerEXT m_DebugMessenger;

private:
	static const std::vector<const char*> s_ValidationLayers;
	static const std::vector<const char*> s_RequiredInstanceExtensions;
	static const std::vector<const char*> s_RequiredDeviceExtensions;
};

