#pragma once

#include "Common.h"
#include <vector>

class InstanceVK
{
	DECL_NO_COPY(InstanceVK);
public:
	InstanceVK();
	~InstanceVK();

	bool finalize(bool validationLayersEnabled);
	void release();

	void addRequiredExtension(const char* extensionName);
	void addOptionalExtension(const char* extensionName);
	void addValidationLayer(const char* layerName);
	
	//GETTERS
	VkInstance getInstance() { return m_Instance; }
	bool validationLayersEnabled() { return m_ValidationLayersEnabled; }
	const std::vector<const char*>& getValidationLayers() { return m_ValidationLayers; }

private:
	bool initInstance();
	bool initializeDebugMessenger();

	bool validationLayersSupported();
	bool setEnabledExtensions();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

private:
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
	
private:
	VkInstance m_Instance;

	std::vector<const char*> m_RequestedRequiredExtensions;
	std::vector<const char*> m_RequestedOptionalExtensions;
	std::vector<const char*> m_EnabledExtensions;
	std::vector<VkExtensionProperties> m_AvailableExtensions;
	
	bool m_ValidationLayersEnabled;
	std::vector<const char*> m_ValidationLayers;

	VkDebugUtilsMessengerEXT m_DebugMessenger;
};

