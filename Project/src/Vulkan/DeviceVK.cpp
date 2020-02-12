#include "DeviceVK.h"
#include "InstanceVK.h"
#include "CopyHandlerVK.h"
#include "CommandBufferVK.h"

#include <iostream>
#include <map>
#include <set>

#define GET_DEVICE_PROC_ADDR(device, function_name) if ((function_name = reinterpret_cast<PFN_##function_name>(vkGetDeviceProcAddr(device, #function_name))) == nullptr) { LOG("--- Vulkan: Failed to load DeviceFunction '%s'", #function_name); }

DeviceVK::DeviceVK() :
	m_PhysicalDevice(VK_NULL_HANDLE),
	m_Device(VK_NULL_HANDLE),
	m_GraphicsQueue(VK_NULL_HANDLE),
	m_ComputeQueue(VK_NULL_HANDLE),
	m_TransferQueue(VK_NULL_HANDLE),
	m_PresentQueue(VK_NULL_HANDLE),
	m_RayTracingProperties({}),
	m_pCopyHandler()
{
}

DeviceVK::~DeviceVK()
{
	release();
}

bool DeviceVK::finalize(InstanceVK* pInstance)
{
	if (!initPhysicalDevice(pInstance))
		return false;

	if (!initLogicalDevice(pInstance))
		return false;

	registerExtensionFunctions();

	m_pCopyHandler = DBG_NEW CopyHandlerVK(this);
	m_pCopyHandler->init();

	std::cout << "--- Device: Vulkan Device created successfully!" << std::endl;
	return true;
}

void DeviceVK::release()
{
	if (m_Device != VK_NULL_HANDLE) 
	{
		vkDeviceWaitIdle(m_Device);
		
		SAFEDELETE(m_pCopyHandler);
	
		vkDestroyDevice(m_Device, nullptr);
		m_Device = VK_NULL_HANDLE;
	}
}

void DeviceVK::addRequiredExtension(const char* extensionName)
{
	m_RequestedRequiredExtensions.push_back(extensionName);
}

void DeviceVK::addOptionalExtension(const char* extensionName)
{
	m_RequestedOptionalExtensions.push_back(extensionName);
}

void DeviceVK::executeCommandBuffer(VkQueue queue, CommandBufferVK* pCommandBuffer, const VkSemaphore* pWaitSemaphore, const VkPipelineStageFlags* pWaitStages, 
	uint32_t waitSemaphoreCount, const VkSemaphore* pSignalSemaphores, uint32_t signalSemaphoreCount)
{
	//Submit
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount	= waitSemaphoreCount;
	submitInfo.pWaitSemaphores		= pWaitSemaphore;
	submitInfo.pWaitDstStageMask	= pWaitStages;

	VkCommandBuffer commandBuffers[] = { pCommandBuffer->getCommandBuffer() };
	submitInfo.pCommandBuffers		= commandBuffers;
	submitInfo.commandBufferCount	= 1;
	submitInfo.signalSemaphoreCount = signalSemaphoreCount;
	submitInfo.pSignalSemaphores	= pSignalSemaphores;

	VkResult result = vkQueueSubmit(queue, 1, &submitInfo, pCommandBuffer->getFence());
	VK_CHECK_RESULT(result, "vkQueueSubmit failed");
}

void DeviceVK::wait()
{
	VkResult result = vkDeviceWaitIdle(m_Device);
	if (result != VK_SUCCESS) 
	{ 
		LOG("vkDeviceWaitIdle failed");
	}
}

bool DeviceVK::initPhysicalDevice(InstanceVK* pInstance)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(pInstance->getInstance() , &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		std::cerr << "--- Device: Presentation is not supported by the selected physicaldevice" << std::endl;
		return false;
	}

	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(pInstance->getInstance(), &deviceCount, physicalDevices.data());

	std::multimap<int32_t, VkPhysicalDevice> physicalDeviceCandidates;

	for (const auto& physicalDevice : physicalDevices)
	{
		int32_t score = rateDevice(physicalDevice);
		physicalDeviceCandidates.insert(std::make_pair(score, physicalDevice));
	}

	// Check if the best candidate is suitable at all
	if (physicalDeviceCandidates.rbegin()->first <= 0)
	{
		std::cerr << "--- Device: Failed to find a suitable GPU!" << std::endl;
		return false;
	}

	m_PhysicalDevice = physicalDeviceCandidates.rbegin()->second;
	setEnabledExtensions();
	m_DeviceQueueFamilyIndices = findQueueFamilies(m_PhysicalDevice);

	return true;
}

bool DeviceVK::initLogicalDevice(InstanceVK* pInstance)
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = 
	{
		m_DeviceQueueFamilyIndices.graphicsFamily.value(),
		m_DeviceQueueFamilyIndices.computeFamily.value(),
		m_DeviceQueueFamilyIndices.transferFamily.value(),
		m_DeviceQueueFamilyIndices.presentFamily.value()
	};

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
	deviceFeatures.fillModeNonSolid = true;
	deviceFeatures.vertexPipelineStoresAndAtomics = true;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_EnabledExtensions.size());
	createInfo.ppEnabledExtensionNames = m_EnabledExtensions.data();

	if (pInstance->validationLayersEnabled())
	{
		auto& validationLayers = pInstance->getValidationLayers();
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS)
	{
		std::cerr << "--- Device: Failed to create logical device!" << std::endl;
		return false;
	}

	vkGetDeviceQueue(m_Device, m_DeviceQueueFamilyIndices.graphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, m_DeviceQueueFamilyIndices.computeFamily.value(), 0, &m_ComputeQueue);
	vkGetDeviceQueue(m_Device, m_DeviceQueueFamilyIndices.transferFamily.value(), 0, &m_TransferQueue);
	vkGetDeviceQueue(m_Device, m_DeviceQueueFamilyIndices.presentFamily.value(), 0, &m_PresentQueue);

	return true;
}

int32_t DeviceVK::rateDevice(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	bool requiredExtensionsSupported = false;
	uint32_t numOfOptionalExtensionsSupported = 0;
	checkExtensionsSupport(physicalDevice, requiredExtensionsSupported, numOfOptionalExtensionsSupported);

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	if (!indices.IsComplete() || !requiredExtensionsSupported)
		return 0;

	int score = 1 + numOfOptionalExtensionsSupported;

	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 1000;

	return score;
}

void DeviceVK::checkExtensionsSupport(VkPhysicalDevice physicalDevice, bool& requiredExtensionsSupported, uint32_t& numOfOptionalExtensionsSupported)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(m_RequestedRequiredExtensions.begin(), m_RequestedRequiredExtensions.end());
	std::set<std::string> optionalExtensions(m_RequestedOptionalExtensions.begin(), m_RequestedOptionalExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
		optionalExtensions.erase(extension.extensionName);
	}

	requiredExtensionsSupported = requiredExtensions.empty();
	numOfOptionalExtensionsSupported = optionalExtensions.size();
}

void DeviceVK::setEnabledExtensions()
{
	m_EnabledExtensions = std::vector<const char*>(m_RequestedRequiredExtensions.begin(), m_RequestedRequiredExtensions.end());

	for (auto& requiredExtensions : m_RequestedRequiredExtensions)
	{
		m_ExtensionsStatus[requiredExtensions] = true;
	}
	
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, nullptr);

	m_AvailabeExtensions.resize(extensionCount);
	vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, m_AvailabeExtensions.data());

	std::set<std::string> optionalExtensions(m_RequestedOptionalExtensions.begin(), m_RequestedOptionalExtensions.end());

	for (const auto& extension : m_AvailabeExtensions)
	{
		if (optionalExtensions.erase(extension.extensionName) > 0)
		{
			m_EnabledExtensions.push_back(extension.extensionName);
			m_ExtensionsStatus[extension.extensionName] = true;
		}
	}

	if (optionalExtensions.size() > 0)
	{
		for (const auto& extension : optionalExtensions)
		{
			std::cerr << "--- Device: Optional Extension [ " << extension << " ] not supported!" << std::endl;
			m_ExtensionsStatus[extension.c_str()] = false;
		}
	}
}

QueueFamilyIndices DeviceVK::findQueueFamilies(VkPhysicalDevice physicalDevice)
{
	QueueFamilyIndices indices;
	
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	indices.graphicsFamily = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, queueFamilies);
	indices.computeFamily = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, queueFamilies);
	indices.transferFamily = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, queueFamilies);
	indices.presentFamily = indices.graphicsFamily; //Assume present support at this stage

	return indices;
}

void DeviceVK::registerExtensionFunctions()
{
	if (m_ExtensionsStatus["VK_NV_ray_tracing"])
	{
		// Get VK_NV_ray_tracing related function pointers
		GET_DEVICE_PROC_ADDR(m_Device, vkCreateAccelerationStructureNV);
		GET_DEVICE_PROC_ADDR(m_Device, vkDestroyAccelerationStructureNV);
		GET_DEVICE_PROC_ADDR(m_Device, vkBindAccelerationStructureMemoryNV);
		GET_DEVICE_PROC_ADDR(m_Device, vkGetAccelerationStructureHandleNV);
		GET_DEVICE_PROC_ADDR(m_Device, vkGetAccelerationStructureMemoryRequirementsNV);
		GET_DEVICE_PROC_ADDR(m_Device, vkCmdBuildAccelerationStructureNV);
		GET_DEVICE_PROC_ADDR(m_Device, vkCreateRayTracingPipelinesNV);
		GET_DEVICE_PROC_ADDR(m_Device, vkGetRayTracingShaderGroupHandlesNV);
		GET_DEVICE_PROC_ADDR(m_Device, vkCmdTraceRaysNV);

		std::cout << "--- Device: Successfully intialized [ VK_NV_ray_tracing ] function pointers!" << std::endl;

		//Query Ray Tracing properties
		m_RayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
		VkPhysicalDeviceProperties2 deviceProps2 = {};
		deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProps2.pNext = &m_RayTracingProperties;
		vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &deviceProps2);
	}
	else
	{
		std::cerr << "--- Device: Failed to intialize [ VK_NV_ray_tracing ] function pointers!" << std::endl;
	}
}

uint32_t DeviceVK::getQueueFamilyIndex(VkQueueFlagBits queueFlags, const std::vector<VkQueueFamilyProperties>& queueFamilies)
{
	if (queueFlags & VK_QUEUE_COMPUTE_BIT)
	{
		for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
		{
			if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				return i;
		}
	}

	if (queueFlags & VK_QUEUE_TRANSFER_BIT)
	{
		for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
		{
			if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				return i;
		}
	}

	for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
	{
		if (queueFamilies[i].queueFlags & queueFlags)
			return i;
	}

	return UINT32_MAX;
}
