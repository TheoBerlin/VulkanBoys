#include "SamplerVK.h"
#include "DeviceVK.h"

SamplerVK::SamplerVK(DeviceVK* pDevice) :
	m_pDevice(pDevice),
	m_Sampler(VK_NULL_HANDLE)
{
}

SamplerVK::~SamplerVK()
{
	if (m_Sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(m_pDevice->getDevice(), m_Sampler, nullptr);
		m_Sampler = VK_NULL_HANDLE;
	}
}

bool SamplerVK::init(const SamplerParams& params)
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType					= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.pNext					= nullptr;
	samplerInfo.flags					= 0;
	samplerInfo.magFilter				= params.MagFilter;
	samplerInfo.minFilter				= params.MinFilter;
	samplerInfo.addressModeU			= params.WrapModeU;
	samplerInfo.addressModeV			= params.WrapModeV;
	samplerInfo.addressModeW			= params.WrapModeW;
	samplerInfo.anisotropyEnable		= VK_FALSE;
	samplerInfo.maxAnisotropy			= 16;
	samplerInfo.borderColor				= VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable			= VK_FALSE;
	samplerInfo.compareOp				= VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode				= VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias				= 0.0f;
	samplerInfo.minLod					= 0.0f;
	samplerInfo.maxLod					= 1.0f;

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateSampler(m_pDevice->getDevice(), &samplerInfo, nullptr, &m_Sampler), "--- SamplerVK: vkCreateSampler failed");
	return true;
}
