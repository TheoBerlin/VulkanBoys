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

#ifdef NDEBUG
#define VALIDATION_LAYERS_ENABLED false
#else
#define VALIDATION_LAYERS_ENABLED true
#endif

#define VK_CHECK_RESULT(_func_call_, _err_msg_) if (_func_call_ != VK_SUCCESS) { std::cerr << _err_msg_ << std::endl; }
#define VK_CHECK_RESULT_RETURN_FALSE(_func_call_, _err_msg_) if (_func_call_ != VK_SUCCESS) { std::cerr << _err_msg_ << std::endl; return false; }

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
constexpr uint32_t VERTEX_BUFFER_DESCRIPTORS_PER_SET_BUNDLE = 3;
constexpr uint32_t CONSTANT_BUFFER_DESCRIPTORS_PER_SET_BUNDLE = 2;
constexpr uint32_t SAMPLER_DESCRIPTORS_PER_SET_BUNDLE = 1;
constexpr uint32_t DESCRIPTOR_SETS_PER_TRIANGLE_PER_FRAME = 2;
constexpr uint32_t DESCRIPTOR_SETS_PER_TRIANGLE = MAX_FRAMES_IN_FLIGHT * DESCRIPTOR_SETS_PER_TRIANGLE_PER_FRAME;
constexpr uint32_t NUM_MATERIALS = 4;

constexpr uint32_t MAX_NUM_DESCRIPTOR_SETS = 1024;// NUM_MATERIALS* DESCRIPTOR_SETS_PER_TRIANGLE;
constexpr uint32_t MAX_NUM_STORAGE_DESCRIPTORS = MAX_NUM_DESCRIPTOR_SETS * 16;///*DESCRIPTOR_SETS_PER_TRIANGLE * */VERTEX_BUFFER_DESCRIPTORS_PER_SET_BUNDLE;
constexpr uint32_t MAX_NUM_UNIFORM_DESCRIPTORS = MAX_NUM_DESCRIPTOR_SETS * 16;///*DESCRIPTOR_SETS_PER_TRIANGLE * */CONSTANT_BUFFER_DESCRIPTORS_PER_SET_BUNDLE;
constexpr uint32_t MAX_NUM_SAMPLER_DESCRIPTORS = MAX_NUM_DESCRIPTOR_SETS * 16;///*DESCRIPTOR_SETS_PER_TRIANGLE * */SAMPLER_DESCRIPTORS_PER_SET_BUNDLE;

class DescriptorCounts
{
public:
    unsigned int m_UniformBuffers, m_StorageBuffers, m_SampledImages;

    void enlarge(unsigned int factor)
    {
        m_UniformBuffers *= factor;
        m_StorageBuffers *= factor;
        m_SampledImages *= factor;
    }

    void operator-=(const DescriptorCounts& other)
    {
        m_UniformBuffers -= other.m_UniformBuffers;
        m_StorageBuffers -= other.m_StorageBuffers;
        m_SampledImages -= other.m_SampledImages;
    }
};
