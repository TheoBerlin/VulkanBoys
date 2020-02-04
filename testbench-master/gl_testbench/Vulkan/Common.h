#pragma once
#ifdef _WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
    #include <vulkan/vulkan.h>
#else
    #error Invalid platform, only Windows is supported
#endif

#define DECL_NO_COPY(Type) \
	Type(Type&&) = delete; \
	Type(const Type&) = delete; \
	Type& operator=(Type&&) = delete; \
	Type& operator=(const Type&) = delete


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

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
constexpr uint32_t VERTEX_BUFFER_DESCRIPTORS_PER_SET_BUNDLE = 3;
constexpr uint32_t CONSTANT_BUFFER_DESCRIPTORS_PER_SET_BUNDLE = 2;
constexpr uint32_t SAMPLER_DESCRIPTORS_PER_SET_BUNDLE = 1;
constexpr uint32_t DESCRIPTOR_SETS_PER_TRIANGLE_PER_FRAME = 2;
constexpr uint32_t DESCRIPTOR_SETS_PER_TRIANGLE = MAX_FRAMES_IN_FLIGHT * DESCRIPTOR_SETS_PER_TRIANGLE_PER_FRAME;
constexpr uint32_t NUM_MATERIALS = 4;

constexpr uint32_t MAX_NUM_DESCRIPTOR_SETS = 256;// NUM_MATERIALS* DESCRIPTOR_SETS_PER_TRIANGLE;
constexpr uint32_t MAX_NUM_STORAGE_DESCRIPTORS = MAX_NUM_DESCRIPTOR_SETS * 16;///*DESCRIPTOR_SETS_PER_TRIANGLE * */VERTEX_BUFFER_DESCRIPTORS_PER_SET_BUNDLE;
constexpr uint32_t MAX_NUM_UNIFORM_DESCRIPTORS = MAX_NUM_DESCRIPTOR_SETS * 16;///*DESCRIPTOR_SETS_PER_TRIANGLE * */CONSTANT_BUFFER_DESCRIPTORS_PER_SET_BUNDLE;
constexpr uint32_t MAX_NUM_SAMPLER_DESCRIPTORS = MAX_NUM_DESCRIPTOR_SETS * 16;///*DESCRIPTOR_SETS_PER_TRIANGLE * */SAMPLER_DESCRIPTORS_PER_SET_BUNDLE;