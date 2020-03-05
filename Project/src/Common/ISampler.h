#pragma once
#include "Core/Core.h"

#include "Vulkan/VulkanCommon.h"

struct SamplerParams
{
	VkFilter MagFilter;
	VkFilter MinFilter;
	VkSamplerAddressMode WrapModeU;
	VkSamplerAddressMode WrapModeV;
	VkSamplerAddressMode WrapModeW;
};

class ISampler
{
public:
	DECL_INTERFACE(ISampler);

	virtual bool init(const SamplerParams& params) = 0;
};