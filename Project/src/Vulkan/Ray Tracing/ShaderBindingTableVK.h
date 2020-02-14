#pragma once

#include "../VulkanCommon.h"

class IGraphicsContext;
class GraphicsContextVK;
class RayTracingPipelineVK;
class DeviceVK;
class PipelineVK;
class BufferVK;

class ShaderBindingTableVK
{
public:
	DECL_NO_COPY(ShaderBindingTableVK);
	
	ShaderBindingTableVK(IGraphicsContext* pContext);
	~ShaderBindingTableVK();

	bool init(RayTracingPipelineVK* pRayTracingPipeline);

	BufferVK* getBuffer() { return m_pSBT; }
	VkDeviceSize getBindingOffsetRaygenShaderGroup() { return m_BindingOffsetRaygenShaderGroup; }
	VkDeviceSize getBindingOffsetMissShaderGroup() { return m_BindingOffsetMissShaderGroup; }
	VkDeviceSize getBindingOffsetHitShaderGroup() { return m_BindingOffsetHitShaderGroup; }
	VkDeviceSize getBindingStride() { return m_BindingStride; }

private:
	GraphicsContextVK* m_pContext;

	BufferVK* m_pSBT;

	VkDeviceSize m_BindingOffsetRaygenShaderGroup;
	VkDeviceSize m_BindingOffsetHitShaderGroup;
	VkDeviceSize m_BindingOffsetMissShaderGroup;
	VkDeviceSize m_BindingStride;
	
};
