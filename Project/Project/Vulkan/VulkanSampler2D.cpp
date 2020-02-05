#include "VulkanSampler2D.h"
#include "VulkanDevice.h"

static VkFilter ConvertFilter(FILTER filter)
{
	if (filter == FILTER::LINEAR)
		return VK_FILTER_LINEAR;
	
	//Default to nearest
	return VK_FILTER_NEAREST;
}

static VkSamplerAddressMode ConvertWrapping(WRAPPING filter)
{
	if (filter == WRAPPING::CLAMP)
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	//Default to nearest
	return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

VulkanSampler2D::VulkanSampler2D(VulkanDevice* pDevice)
	: m_pDevice(pDevice),
	m_Sampler(VK_NULL_HANDLE),
	m_FilterMode(),
	m_WrappingMode()
{
	m_WrappingMode.s = m_WrappingMode.t = WRAPPING::CLAMP;
	m_FilterMode.Mag = m_FilterMode.Min = FILTER::POINT_SAMPLER;
	finalize();
}

VulkanSampler2D::~VulkanSampler2D()
{
	if (m_pDevice->getDevice() != VK_NULL_HANDLE)
		vkDeviceWaitIdle(m_pDevice->getDevice());
	
	if (m_Sampler != VK_NULL_HANDLE)
		destroySampler();
}

void VulkanSampler2D::setMagFilter(FILTER filter)
{
	m_FilterMode.Mag = filter;
	finalize();
}

void VulkanSampler2D::setMinFilter(FILTER filter)
{
	m_FilterMode.Min = filter;
	finalize();
}

void VulkanSampler2D::setWrap(WRAPPING s, WRAPPING t)
{
	m_WrappingMode.s = s;
	m_WrappingMode.t = t;
	finalize();
}

void VulkanSampler2D::destroySampler()
{
	vkDestroySampler(m_pDevice->getDevice(), m_Sampler, nullptr);
	m_Sampler = VK_NULL_HANDLE;

	std::cout << "Destroyed Sampler" << std::endl;
}

void VulkanSampler2D::finalize()
{
	if (m_Sampler != VK_NULL_HANDLE)
		destroySampler();

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType		= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.pNext		= nullptr;
	samplerInfo.flags		= 0;
	samplerInfo.magFilter	= ConvertFilter(m_FilterMode.Mag);
	samplerInfo.minFilter	= ConvertFilter(m_FilterMode.Min);
	samplerInfo.addressModeU = ConvertWrapping(m_WrappingMode.s);
	samplerInfo.addressModeV = ConvertWrapping(m_WrappingMode.t);
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy	= 16;
	samplerInfo.borderColor		= VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable	= VK_FALSE;
	samplerInfo.compareOp	= VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode	= VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias	= 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	
	VkResult result = vkCreateSampler(m_pDevice->getDevice(), &samplerInfo, nullptr, &m_Sampler);
	if (result != VK_SUCCESS) 
	{
		std::cout << "vkCreateSampler failed" << std::endl;
	}
	else
	{
		std::cout << "Created Sampler" << std::endl;
	}
}
