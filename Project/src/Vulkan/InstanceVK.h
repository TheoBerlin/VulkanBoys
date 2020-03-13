#pragma once

#include "VulkanCommon.h"
#include <vector>

class InstanceVK
{
public:
	InstanceVK();
	~InstanceVK();

	DECL_NO_COPY(InstanceVK);

	bool finalize(bool validationLayersEnabled);
	void release();

	void debugPrintAvailableExtensions();
	void debugPrintAvailableLayers();

	void addRequiredExtension(const char* extensionName);
	void addOptionalExtension(const char* extensionName);
	void addValidationLayer(const char* layerName);
	
	//GETTERS
	bool							validationLayersEnabled()	{ return m_ValidationLayersEnabled; }
	VkInstance						getInstance()				{ return m_Instance; }
	const std::vector<const char*>& getValidationLayers()		{ return m_ValidationLayers; }

private:
	bool initInstance();
	bool initializeDebugMessenger();

	bool validationLayersSupported();
	bool setEnabledExtensions();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	void retriveAvailableExtensions();
	void retriveAvailableLayers();

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
	std::vector<VkLayerProperties> m_AvailableLayers;
	std::vector<const char*> m_ValidationLayers;
	
	bool m_ValidationLayersEnabled;
	bool m_HasRetrivedExtensions;
	bool m_HasRetrivedLayers;

	VkDebugUtilsMessengerEXT m_DebugMessenger;
};

