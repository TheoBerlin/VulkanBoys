#pragma once
#include "Core/Core.h"

#include "Vulkan/VulkanCommon.h"

struct SamplerParams
{
	VkFilter MagFilter;
	VkFilter MinFilter;
	VkSamplerAddressMode WrapModeS;
	VkSamplerAddressMode WrapModeT;
};

class ISampler
{
public:
	DECL_INTERFACE(ISampler);

	virtual bool init(const SamplerParams& params) = 0;
};