#pragma once
#include "VulkanCommon.h"

#include <vector>

class DeviceVK;
class RenderPassVK;
class PipelineLayoutVK;

class PipelineVK
{
public:
	PipelineVK(DeviceVK* pDevice);
	~PipelineVK();

	void addVertexBinding(uint32_t binding, VkVertexInputRate inputRate, uint32_t stride);
	void addVertexAttribute(uint32_t binding, VkFormat format, uint32_t location, uint32_t offset);
	void addColorBlendAttachment(const VkPipelineColorBlendAttachmentState& colorBlendAttachment);

	void setInputAssembly(VkPrimitiveTopology topology, bool primitiveRestartEnable);
	void setRasterizerState(const VkPipelineRasterizationStateCreateInfo& rasterizerState);
	void setDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& depthStencilState);
	void setBlendState(VkLogicOp logicOp, bool logicOpEnable, float blendConstants[4]);

	bool finalizeGraphics(const std::vector<const IShader*>& shaders, const RenderPassVK* pRenderPass, const PipelineLayoutVK* pPipelineLayout);
	bool finalizeCompute(const IShader* shader, const PipelineLayoutVK* pPipelineLayout);

	VkPipeline getPipeline() const { return m_Pipeline; }
	VkPipelineBindPoint getBindPoint() const { return m_BindPoint; }

private:
    void createShaderStageInfo(VkPipelineShaderStageCreateInfo& shaderStageInfo, const IShader* shader);

private:
	std::vector<VkVertexInputBindingDescription> m_VertexBindings;
	std::vector<VkVertexInputAttributeDescription> m_VertexAttributes;
	std::vector< VkPipelineColorBlendAttachmentState> m_ColorBlendAttachments;
	VkPipelineInputAssemblyStateCreateInfo m_InputAssembly;
	VkPipelineRasterizationStateCreateInfo m_RasterizerState;
	VkPipelineMultisampleStateCreateInfo m_MultisamplingState;
	VkPipelineColorBlendStateCreateInfo m_BlendState;
	VkPipelineDepthStencilStateCreateInfo m_DepthStencilState;
	DeviceVK* m_pDevice;
	VkPipeline m_Pipeline;
	VkPipelineBindPoint m_BindPoint;
};
