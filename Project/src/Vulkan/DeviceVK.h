#pragma once
#include "VulkanCommon.h"

#include <optional>
#include <vector>
#include <unordered_map>

class InstanceVK;
class CommandBufferVK;
class CopyHandlerVK;

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

	void executePrimaryCommandBuffer(VkQueue queue, CommandBufferVK* pCommandBuffer, const VkSemaphore* pWaitSemaphore, const VkPipelineStageFlags* pWaitStages,
		uint32_t waitSemaphoreCount, const VkSemaphore* pSignalSemaphores, uint32_t signalSemaphoreCount);
	void executeSecondaryCommandBuffer(CommandBufferVK* pPrimaryCommandBuffer, CommandBufferVK* pSecondaryCommandBuffer);
	void wait();

	//GETTERS
	VkPhysicalDevice getPhysicalDevice() { return m_PhysicalDevice; };
	VkDevice getDevice() { return m_Device; }
	
	VkQueue getGraphicsQueue() { return m_GraphicsQueue; }
	VkQueue getComputeQueue() { return m_ComputeQueue; }
	VkQueue getTransferQueue() { return m_TransferQueue; }
	VkQueue getPresentQueue() { return m_PresentQueue; }

	CopyHandlerVK* getCopyHandler() const { return m_pCopyHandler; }

	const QueueFamilyIndices& getQueueFamilyIndices() const { return m_DeviceQueueFamilyIndices; }
	bool hasUniqueQueueFamilyIndices() const;
	
	const VkPhysicalDeviceRayTracingPropertiesNV& getRayTracingProperties() const { return m_RayTracingProperties; }
	bool supportsRayTracing() const { return m_ExtensionsStatus.at(VK_NV_RAY_TRACING_EXTENSION_NAME); }

private:
	bool initPhysicalDevice(InstanceVK* pInstance);
	bool initLogicalDevice(InstanceVK* pInstance);

	int32_t rateDevice(VkPhysicalDevice physicalDevice);
	void checkExtensionsSupport(VkPhysicalDevice physicalDevice, bool& requiredExtensionsSupported, uint32_t& numOfOptionalExtensionsSupported);
	void setEnabledExtensions();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);

	void registerExtensionFunctions();
	
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
	std::unordered_map<std::string, bool> m_ExtensionsStatus;
	std::vector<VkExtensionProperties> m_AvailabeExtensions;

	CopyHandlerVK* m_pCopyHandler;

	//Extensions
	VkPhysicalDeviceRayTracingPropertiesNV m_RayTracingProperties;

public:
	//Extension Function Pointers
	PFN_vkCreateAccelerationStructureNV					vkCreateAccelerationStructureNV;
	PFN_vkDestroyAccelerationStructureNV				vkDestroyAccelerationStructureNV;
	PFN_vkBindAccelerationStructureMemoryNV				vkBindAccelerationStructureMemoryNV;
	PFN_vkGetAccelerationStructureHandleNV				vkGetAccelerationStructureHandleNV;
	PFN_vkGetAccelerationStructureMemoryRequirementsNV	vkGetAccelerationStructureMemoryRequirementsNV;
	PFN_vkCmdBuildAccelerationStructureNV				vkCmdBuildAccelerationStructureNV;
	PFN_vkCreateRayTracingPipelinesNV					vkCreateRayTracingPipelinesNV;
	PFN_vkGetRayTracingShaderGroupHandlesNV				vkGetRayTracingShaderGroupHandlesNV;
	PFN_vkCmdTraceRaysNV								vkCmdTraceRaysNV;
};

