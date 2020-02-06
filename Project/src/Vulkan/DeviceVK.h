#pragma once

#include <optional>
#include <vector>

#include "VulkanCommon.h"

class InstanceVK;

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> computeFamily;
	std::optional<uint32_t> transferFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete()
	{
		return graphicsFamily.has_value() && computeFamily.has_value() && transferFamily.has_value() && presentFamily.has_value();
	}
};

class DeviceVK
{
	DECL_NO_COPY(DeviceVK);
	
public:
	DeviceVK();
	~DeviceVK();

	bool finalize(InstanceVK* pInstance);
	void release();

	void addRequiredExtension(const char* extensionName);
	void addOptionalExtension(const char* extensionName);

	//GETTERS
	VkPhysicalDevice getPhysicalDevice() { return m_PhysicalDevice; };
	VkDevice getDevice() { return m_Device; }
	
	VkQueue getGraphicsQueue() { return m_GraphicsQueue; }
	VkQueue getComputeQueue() { return m_ComputeQueue; }
	VkQueue getTransferQueue() { return m_TransferQueue; }
	VkQueue getPresentQueue() { return m_PresentQueue; }

private:
	bool initPhysicalDevice(InstanceVK* pInstance);
	bool initLogicalDevice(InstanceVK* pInstance);

	int32_t rateDevice(VkPhysicalDevice physicalDevice);
	void checkExtensionsSupport(VkPhysicalDevice physicalDevice, bool& requiredExtensionsSupported, uint32_t& numOfOptionalExtensionsSupported);
	void setEnabledExtensions();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);

private:
	static uint32_t getQueueFamilyIndex(VkQueueFlagBits queueFlags, const std::vector<VkQueueFamilyProperties>& queueFamilies);
	
private:	
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_Device;

	QueueFamilyIndices m_DeviceQueueFamilyIndices;
	
	VkQueue m_GraphicsQueue;
	VkQueue m_ComputeQueue;
	VkQueue m_TransferQueue;
	VkQueue m_PresentQueue;

	std::vector<const char*> m_RequestedRequiredExtensions;
	std::vector<const char*> m_RequestedOptionalExtensions;
	std::vector<const char*> m_EnabledExtensions;
};

