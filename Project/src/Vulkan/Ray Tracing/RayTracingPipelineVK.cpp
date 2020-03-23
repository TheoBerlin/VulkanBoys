#include "RayTracingPipelineVK.h"

#include "Vulkan/Ray Tracing/ShaderBindingTableVK.h"
#include "Vulkan/GraphicsContextVK.h"
#include "Vulkan/DeviceVK.h"
#include "Vulkan/ShaderVK.h"
#include "Vulkan/PipelineLayoutVK.h"

RayTracingPipelineVK::RayTracingPipelineVK(GraphicsContextVK* pGraphicsContext) :
	m_pGraphicsContext(pGraphicsContext),
	m_MaxRecursionDepth(m_pGraphicsContext->getDevice()->getRayTracingProperties().maxRecursionDepth),
	m_Pipeline(VK_NULL_HANDLE),
	m_pSBT(nullptr)
{
}

RayTracingPipelineVK::~RayTracingPipelineVK()
{
	if (m_Pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(m_pGraphicsContext->getDevice()->getDevice(), m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDLE;
	}

	SAFEDELETE(m_pSBT);
}

void RayTracingPipelineVK::addRaygenShaderGroup(const RaygenGroupParams& params)
{
	m_Shaders.push_back(params.pRaygenShader);

	VkRayTracingShaderGroupCreateInfoNV shaderGroupCreateInfo = {};
	shaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	shaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	shaderGroupCreateInfo.generalShader = m_Shaders.size() - 1;
	shaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_NV;
	shaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_NV;
	shaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_NV;
	m_RaygenShaderGroups.push_back(shaderGroupCreateInfo);
}

void RayTracingPipelineVK::addMissShaderGroup(const MissGroupParams& params)
{
	m_Shaders.push_back(params.pMissShader);

	VkRayTracingShaderGroupCreateInfoNV shaderGroupCreateInfo = {};
	shaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	shaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	shaderGroupCreateInfo.generalShader = m_Shaders.size() - 1;
	shaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_NV;
	shaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_NV;
	shaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_NV;
	m_MissShaderGroups.push_back(shaderGroupCreateInfo);
}

void RayTracingPipelineVK::addHitShaderGroup(const HitGroupParams& params)
{
	VkRayTracingShaderGroupCreateInfoNV shaderGroupCreateInfo = {};
	shaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	shaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
	shaderGroupCreateInfo.generalShader = VK_SHADER_UNUSED_NV;
	shaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_NV;
	
	if (params.pAnyHitShader != nullptr)
	{
		m_Shaders.push_back(params.pAnyHitShader);
		shaderGroupCreateInfo.anyHitShader = m_Shaders.size() - 1;
	}
	else
	{
		shaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_NV;
	}

	if (params.pClosestHitShader != nullptr)
	{
		m_Shaders.push_back(params.pClosestHitShader);
		shaderGroupCreateInfo.closestHitShader = m_Shaders.size() - 1;
	}
	else
	{
		LOG("--- RayTracingPipeline: Failed to add Hit Shader Group, Closest Hit Shader must be valid!");
		return;
	}
	
	m_HitShaderGroups.push_back(shaderGroupCreateInfo);
}

bool RayTracingPipelineVK::finalize(PipelineLayoutVK* pPipelineLayout)
{
	// Define shader stage create infos
	m_ShaderStagesInfos.reserve(m_Shaders.size());

	for (ShaderVK* pShader : m_Shaders)
	{
		VkPipelineShaderStageCreateInfo shaderStageInfo;
		createShaderStageInfo(shaderStageInfo, pShader);
		m_ShaderStagesInfos.push_back(shaderStageInfo);
	}

	for (auto& raygenShaderGroup : m_RaygenShaderGroups)
	{
		m_AllShaderGroups.push_back(raygenShaderGroup);
	}

	for (auto& missShaderGroup : m_MissShaderGroups)
	{
		m_AllShaderGroups.push_back(missShaderGroup);
	}

	for (auto& hitShaderGroup : m_HitShaderGroups)
	{
		m_AllShaderGroups.push_back(hitShaderGroup);
	}

	/*uint32_t numGroups = m_RaygenShaderGroups.size() + m_IntersectShaderGroups.size() + m_MissShaderGroups.size();
	m_AllShaderGroups.resize(numGroups);

	size_t rayGenDst = (size_t)m_AllShaderGroups.data();
	size_t missDst = rayGenDst + m_RaygenShaderGroups.size() * sizeof(VkRayTracingShaderGroupCreateInfoNV);
	size_t intersectDst = missDst + m_MissShaderGroups.size() * sizeof(VkRayTracingShaderGroupCreateInfoNV);
	
	memcpy((void*)rayGenDst, m_RaygenShaderGroups.data(), m_RaygenShaderGroups.size() * sizeof(VkRayTracingShaderGroupCreateInfoNV));
	memcpy((void*)missDst, m_MissShaderGroups.data(), m_MissShaderGroups.size() * sizeof(VkRayTracingShaderGroupCreateInfoNV));
	memcpy((void*)intersectDst, m_IntersectShaderGroups.data(), m_IntersectShaderGroups.size() * sizeof(VkRayTracingShaderGroupCreateInfoNV));*/

	VkRayTracingPipelineCreateInfoNV rayPipelineInfo{};
	rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
	rayPipelineInfo.stageCount = m_ShaderStagesInfos.size();
	rayPipelineInfo.pStages = m_ShaderStagesInfos.data();
	rayPipelineInfo.groupCount = m_AllShaderGroups.size();
	rayPipelineInfo.pGroups = m_AllShaderGroups.data();
	rayPipelineInfo.maxRecursionDepth = m_MaxRecursionDepth;
	rayPipelineInfo.layout = pPipelineLayout->getPipelineLayout();
	
	VK_CHECK_RESULT_RETURN_FALSE(m_pGraphicsContext->getDevice()->vkCreateRayTracingPipelinesNV(m_pGraphicsContext->getDevice()->getDevice(), VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &m_Pipeline), "--- RayTracingPipeline: Failed to create RayTracingPipeline!");

	LOG("--- RayTracingPipeline: Successfully created RayTracingPipeline!");

	m_pSBT = DBG_NEW ShaderBindingTableVK(m_pGraphicsContext);
	m_pSBT->init(this);

	return true;
}

void RayTracingPipelineVK::createShaderStageInfo(VkPipelineShaderStageCreateInfo& shaderStageInfo, const ShaderVK* pShader)
{
	shaderStageInfo = {};

	shaderStageInfo.stage = convertShaderType(pShader->getShaderType());
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.pNext = nullptr;
	shaderStageInfo.flags = 0;
	shaderStageInfo.stage = convertShaderType(pShader->getShaderType());
	shaderStageInfo.module = pShader->getShaderModule();
	shaderStageInfo.pName = pShader->getEntryPoint().c_str();
	shaderStageInfo.pSpecializationInfo = pShader->getSpecializationInfo();
}