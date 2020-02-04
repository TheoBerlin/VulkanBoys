#pragma once
#include "../Sampler2D.h"
#include "Common.h"

class VulkanDevice;

class VulkanSampler2D : public Sampler2D
{
public:
	VulkanSampler2D(VulkanDevice* pDevice);
	~VulkanSampler2D();

	DECL_NO_COPY(VulkanSampler2D);

	virtual void setMagFilter(FILTER filter) override;
	virtual void setMinFilter(FILTER filter) override;
	virtual void setWrap(WRAPPING s, WRAPPING t) override;

	VkSampler getSampler() const { return m_Sampler; }
private:
	void destroySampler();
	void finalize();
private:
	VulkanDevice* m_pDevice;
	VkSampler m_Sampler;
	struct
	{
		FILTER Min;
		FILTER Mag;
	} m_FilterMode;
	struct
	{
		WRAPPING s;
		WRAPPING t;
	} m_WrappingMode;
};

