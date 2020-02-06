#pragma once
#include "Core/Core.h"
#include "Common/IShader.h"

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

static VkShaderStageFlagBits convertShaderType(EShader shader)
{
    switch (shader)
    {
    case EShader::VERTEX_SHADER:    return VK_SHADER_STAGE_VERTEX_BIT;
    case EShader::GEOMETRY_SHADER:  return VK_SHADER_STAGE_GEOMETRY_BIT;
    case EShader::HULL_SHADER:      return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case EShader::DOMAIN_SHADER:    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case EShader::PIXEL_SHADER:     return VK_SHADER_STAGE_FRAGMENT_BIT;
    case EShader::COMPUTE_SHADER:   return VK_SHADER_STAGE_COMPUTE_BIT;
    }
}

#define VK_CHECK_RESULT(_func_call_, _err_msg_) if (_func_call_ != VK_SUCCESS) { std::cerr << _err_msg_ << std::endl; }
#define VK_CHECK_RESULT_RETURN_FALSE(_func_call_, _err_msg_) if (_func_call_ != VK_SUCCESS) { std::cerr << _err_msg_ << std::endl; return false; }

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;