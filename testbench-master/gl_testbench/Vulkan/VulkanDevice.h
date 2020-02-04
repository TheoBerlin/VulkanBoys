#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <vector>
#include <iostream>
#include <map>
#include <optional>

#include "Common.h"
#include "../ConsoleHelper.h"

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

	void initialize(const char applicationName[], uint32_t vertexBufferDescriptorCount, uint32_t constantBufferDescriptorCount, uint32_t samplerDescriptorCount, uint32_t descriptorSetCount);
	void cleanDescriptorPools(uint32_t frameIndex);
	void reallocDescriptorPool(uint32_t frameIndex);
	void release();

	VkInstance getInstance() { return m_VKInstance; }
	const VkPhysicalDeviceProperties& getPhysicalDeviceProperties() { return m_PhysicalDeviceProperties; }
	VkPhysicalDevice getPhysicalDevice() { return m_PhysicalDevice; }
	VkDevice getDevice() { return m_Device; }
	QueueFamilyIndices getQueueFamilyIndices() { return m_DeviceQueueFamilyIndices; }

	VkQueue getGraphicsQueue() { return m_GraphicsQueue; }
	VkQueue getPresentQueue() { return m_PresentQueue; }

	VkDescriptorPool getDescriptorPool(uint32_t frameIndex) { return m_DescriptorPools[frameIndex]; }

private:
	void initializeInstance(const char applicationName[]);
	void initializePhysicalDevice();
	void initializeLogicalDevice();
	void initializeDebugMessenger();
	void initializeDescriptorPool(uint32_t frameIndex, uint32_t vertexBufferDescriptorCount, uint32_t constantBufferDescriptorCount, uint32_t samplerDescriptorCount, uint32_t descriptorSetCount);
	
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

	VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_Device;
	
	QueueFamilyIndices m_DeviceQueueFamilyIndices;

	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

	VkDebugUtilsMessengerEXT m_DebugMessenger;

	VkDescriptorPool m_DescriptorPools[MAX_FRAMES_IN_FLIGHT];
	std::vector<VkDescriptorPool> m_GarbageDescriptorPools[MAX_FRAMES_IN_FLIGHT];
private:
	static const std::vector<const char*> s_ValidationLayers;
	static const std::vector<const char*> s_RequiredInstanceExtensions;
	static const std::vector<const char*> s_RequiredDeviceExtensions;
};

