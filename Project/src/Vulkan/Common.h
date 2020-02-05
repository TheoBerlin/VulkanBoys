#pragma once
#include "Core/Core.h"

#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
    #include <vulkan/vulkan.h>
#else
    #error Invalid platform, only Windows is supported
#endif

static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) 
{
    VkPhysicalDeviceMemoryProperties memProperties = {};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
        {
            return i;
        }
    }

    return UINT32_MAX;
}

#define VK_CHECK_RESULT(_func_call_, _err_msg_) if (_func_call_ != VK_SUCCESS) { std::cerr << _err_msg_ << std::endl; }
#define VK_CHECK_RESULT_RETURN_FALSE(_func_call_, _err_msg_) if (_func_call_ != VK_SUCCESS) { std::cerr << _err_msg_ << std::endl; return false; }

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;