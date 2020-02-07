#pragma once
#include "../RenderState.h"
#include "VulkanCommon.h"

class VulkanDevice;
class VulkanMaterial;

class VulkanRenderState : public RenderState
{
public:
	VulkanRenderState(VulkanDevice* pDevice);
	~VulkanRenderState();

	DECL_NO_COPY(VulkanRenderState);

	virtual void setWireFrame(bool wireframe) override;
	virtual void set() override;

	void finalize(VulkanMaterial* pMaterial, VkRenderPass renderPass, VkPipelineLayout pipelineLayout);

	VkPipeline getPipelineState() { return m_Pipeline; }
private:
	VulkanDevice* m_pDevice;
	VkPipeline m_Pipeline;
	bool m_WireFrame;
};

