#include "RenderPassVK.h"
#include "DeviceVK.h"

RenderPassVK::RenderPassVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_RenderPass(VK_NULL_HANDLE)
{
}

RenderPassVK::~RenderPassVK()
{
	if (m_RenderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(m_pDevice->getDevice(), m_RenderPass, nullptr);
		m_RenderPass = VK_NULL_HANDLE;
	}
}

void RenderPassVK::addAttachment(const VkAttachmentDescription& attachmentDesc)
{
	m_Attachments.emplace_back(attachmentDesc);
}

void RenderPassVK::addSubpass(const VkAttachmentReference* pColorAttachments, uint32_t colorAttachmentCount, const VkAttachmentReference* pDepthStencilAttachment)
{
	VkSubpassDescription subpass = {};
	subpass.flags = 0;
	subpass.inputAttachmentCount	= 0;
	subpass.pInputAttachments		= nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments	= nullptr;
	subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount	= colorAttachmentCount;
	subpass.pColorAttachments		= pColorAttachments;
	subpass.pDepthStencilAttachment = pDepthStencilAttachment;
	subpass.pResolveAttachments		= nullptr;
	m_Subpasses.emplace_back(subpass);
}

void RenderPassVK::addSubpassDependency(const VkSubpassDependency& dependency)
{
	m_SubpassDependencies.emplace_back(dependency);
}

bool RenderPassVK::finalize()
{
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext			= nullptr;
	renderPassInfo.flags			= 0;
	renderPassInfo.attachmentCount	= uint32_t(m_Attachments.size());
	renderPassInfo.pAttachments		= (m_Attachments.data()) ? m_Attachments.data() : nullptr;
	renderPassInfo.subpassCount		= uint32_t(m_Subpasses.size());
	renderPassInfo.pSubpasses		= (m_Subpasses.data()) ? m_Subpasses.data() : nullptr;
	renderPassInfo.dependencyCount	= uint32_t(m_SubpassDependencies.size());
	renderPassInfo.pDependencies	= (m_SubpassDependencies.data()) ? m_SubpassDependencies.data() : nullptr;

	VK_CHECK_RESULT_RETURN_FALSE(vkCreateRenderPass(m_pDevice->getDevice(), &renderPassInfo, nullptr, &m_RenderPass), "vkCreateRenderPass failed");

	m_Subpasses.clear();

	D_LOG("--- RenderPass: Vulkan RenderPass created successfully");
	return true;
}
