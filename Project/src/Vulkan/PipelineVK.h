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
	void addColorBlendAttachment(bool blendEnable, VkColorComponentFlags colorWriteMask);

	void setDepthTest(bool depthTest) { m_DepthTest = depthTest; }
	void setCulling(bool culling) { m_Culling = culling; }
	void setWireFrame(bool wireframe) { m_WireFrame = wireframe; }

	// Creates a graphics pipeline
	bool finalize(const std::vector<IShader*>& shaders, RenderPassVK* pRenderPass, PipelineLayoutVK* pPipelineLayout);
	// Creates a compute pipeline
	bool finalizeCompute(IShader* shader, PipelineLayoutVK* pPipelineLayout);

	VkPipeline getPipeline() const { return m_Pipeline; }

private:
    void createShaderStageInfo(VkPipelineShaderStageCreateInfo& shaderStageInfo, const IShader* shader);

private:
	std::vector<VkVertexInputBindingDescription> m_VertexBindings;
	std::vector<VkVertexInputAttributeDescription> m_VertexAttributes;
	std::vector< VkPipelineColorBlendAttachmentState> m_ColorBlendAttachments;
	DeviceVK* m_pDevice;
	VkPipeline m_Pipeline;
	bool m_WireFrame;
	bool m_Culling;
	bool m_DepthTest;
};
