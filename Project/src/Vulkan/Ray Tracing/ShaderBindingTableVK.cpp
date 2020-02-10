#include "ShaderBindingTableVK.h"

#include "RayTracingPipelineVK.h"
#include "Vulkan/DeviceVK.h"
#include "Vulkan/PipelineVK.h"
#include "Vulkan/BufferVK.h"
#include "Vulkan/GraphicsContextVK.h"

ShaderBindingTableVK::ShaderBindingTableVK(IGraphicsContext* pContext) :
	m_pContext(reinterpret_cast<GraphicsContextVK*>(pContext)),
	m_pSBT(nullptr)
{
}

ShaderBindingTableVK::~ShaderBindingTableVK()
{
	if (m_pSBT != nullptr)
	{
		delete m_pSBT;
		m_pSBT = nullptr;
	}
}

bool ShaderBindingTableVK::init(RayTracingPipelineVK* pRayTracingPipeline)
{
	// Create buffer for the shader binding table
	uint32_t shaderGroupHandleSize = m_pContext->getDevice()->getRayTracingProperties().shaderGroupHandleSize;
	uint32_t sbtSize = shaderGroupHandleSize * pRayTracingPipeline->getNumTotalShaderGroups();
	
	BufferParams sbtParams = {};
	sbtParams.Usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_NV;
	sbtParams.MemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	sbtParams.SizeInBytes = sbtSize;
	
	m_pSBT = reinterpret_cast<BufferVK*>(m_pContext->createBuffer());
	m_pSBT->init(sbtParams);

	// Get shader identifiers
	void* mappedMemory;
	m_pSBT->map(&mappedMemory);
	
	if (m_pContext->getDevice()->vkGetRayTracingShaderGroupHandlesNV(m_pContext->getDevice()->getDevice(), pRayTracingPipeline->getPipeline(), 0, pRayTracingPipeline->getNumTotalShaderGroups(), sbtSize, mappedMemory) != VK_SUCCESS)
	{
		LOG("--- ShaderBindingTable: Failed to get Ray Tracing Shader Group Handles");
		m_pSBT->unmap();
		return false;
	}
	m_pSBT->unmap();

	m_BindingOffsetRaygenShaderGroup = 0;
	m_BindingOffsetMissShaderGroup = pRayTracingPipeline->getNumRaygenShaderGroups() * shaderGroupHandleSize;
	m_BindingOffsetIntersectShaderGroup = m_BindingOffsetMissShaderGroup + pRayTracingPipeline->getNumMissShaderGroups() * shaderGroupHandleSize;
	m_BindingStride = shaderGroupHandleSize; //Todo: Is this even correct?
	
	LOG("--- ShaderBindingTable: Successfully created ShaderBindingTable!");
	return true;
}