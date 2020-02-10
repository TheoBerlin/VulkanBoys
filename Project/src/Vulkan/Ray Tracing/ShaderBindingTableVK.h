#pragma once

#include "../VulkanCommon.h"

class IGraphicsContext;
class GraphicsContextVK;
class RayTracingPipelineVK;
class DeviceVK;
class PipelineVK;
class BufferVK;

#define DEFAULT_RAYGEN_GROUP_INDEX 0
#define DEFAULT_INTERSECT_GROUP_INDEX 1
#define DEFAULT_MISS_GROUP_INDEX 2

class ShaderBindingTableVK
{
public:
	DECL_NO_COPY(ShaderBindingTableVK);
	
	ShaderBindingTableVK(IGraphicsContext* pContext);
	~ShaderBindingTableVK();

	bool finalize(RayTracingPipelineVK* pRayTracingPipeline);

private:
	GraphicsContextVK* m_pContext;

	BufferVK* m_pSBT;
	
};
