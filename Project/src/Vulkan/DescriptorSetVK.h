#pragma once

#include "vulkan/vulkan.h"

class DescriptorSetVK
{
public:
    DescriptorSetVK();
    ~DescriptorSetVK();

    

private:
    VkDescriptorPool m_DescriptorPool;
};
