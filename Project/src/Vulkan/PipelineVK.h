#pragma once
#include "VulkanCommon.h"

#include <vector>

class DeviceVK;
class RenderPassVK;
class PipelineLayoutVK;

class PipelineVK
{
public:
	PipelineVK();
	~PipelineVK();

	void setWireFrame(bool wireframe) { m_WireFrame = wireframe; }

	void create(std::vector<IShader*> shaders, RenderPassVK* pRenderPass, PipelineLayoutVK* pPipelineLayout, DeviceVK* pDevice);

	VkPipeline getPipeline() const { return m_Pipeline; }

private:
    void createShaderStageInfo(VkPipelineShaderStageCreateInfo& shaderStageInfo, const IShader& shader);

private:
	DeviceVK* m_pDevice;
	VkPipeline m_Pipeline;
	bool m_WireFrame;
};
