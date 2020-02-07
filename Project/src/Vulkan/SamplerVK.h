#pragma once

#include "VulkanCommon.h"

class DeviceVK;

class SamplerVK
{
public:
	DECL_NO_COPY(SamplerVK);
	
	SamplerVK(DeviceVK* pDevice);
	~SamplerVK();

	bool init(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode wrapModeS, VkSamplerAddressMode wrapModeT);

	VkSampler getSampler() { return m_Sampler; }

private:
	DeviceVK* m_pDevice;
	
	VkSampler m_Sampler;
};
