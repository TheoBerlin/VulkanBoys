#pragma once
#include "VulkanCommon.h"

#include <vector>

class DeviceVK;

class RenderPassVK
{
public:
	RenderPassVK(DeviceVK* pDevice);
	~RenderPassVK();

	DECL_NO_COPY(RenderPassVK);

	//Add attachments in order. They will be referenced with the index 0, 1, .. and so on. Order matters.
	void addAttachment(const VkAttachmentDescription& attachmentDesc);
	//Add cubpasses in order. They will be referenced with the index 0, 1, .. and so on. Order matters.
	void addSubpass(const VkAttachmentReference* const pColorAttachments, uint32_t colorAttachmentCount, const VkAttachmentReference* const pDepthStencilAttachment);
	void addSubpassDependency(const VkSubpassDependency& dependency);
	bool finalize();

	VkRenderPass getRenderPass() { return m_RenderPass; }

private:
	DeviceVK* m_pDevice;
	std::vector<VkSubpassDependency> m_SubpassDependencies;
	std::vector<VkSubpassDescription> m_Subpasses;
	std::vector<VkAttachmentDescription> m_Attachments;
	VkRenderPass m_RenderPass;
};

