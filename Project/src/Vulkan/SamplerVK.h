#pragma once
#include "Common/ISampler.h"

#include "VulkanCommon.h"

class DeviceVK;

class SamplerVK : public ISampler
{
public:
	SamplerVK(DeviceVK* pDevice);
	~SamplerVK();

	virtual bool init(const SamplerParams& params) override;

	VkSampler getSampler() { return m_Sampler; }

private:
	DeviceVK* m_pDevice;
	VkSampler m_Sampler;
};
