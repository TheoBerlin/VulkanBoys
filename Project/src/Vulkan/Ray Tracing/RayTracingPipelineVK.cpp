#include "RayTracingPipelineVK.h"

#include "Vulkan/DeviceVK.h"
#include "Vulkan/ShaderVK.h"
#include "Vulkan/PipelineLayoutVK.h"

RayTracingPipelineVK::RayTracingPipelineVK(DeviceVK* pDevice) :
	m_pDevice(pDevice),
	m_MaxRecursionDepth(1),
	m_Pipeline(VK_NULL_HANDLE)
{
}

RayTracingPipelineVK::~RayTracingPipelineVK()
{
	if (m_Pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(m_pDevice->getDevice(), m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDLE;
	}
}

void RayTracingPipelineVK::addRaygenShaderGroup(const RaygenGroupParams& params)
{
	if (m_RaygenShaderGroups.size() > 0)
	{
		std::cerr << "--- RayTracingPipeline: Failed to add Raygen Shader Group, currently a maximum of one shader group per type is allowed!" << std::endl;
		return;
	}
	
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
	if (m_MissShaderGroups.size() > 0)
	{
		LOG("--- RayTracingPipeline: Failed to add Miss Shader Group, currently a maximum of one shader group per type is allowed!");
		return;
	}
	
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

void RayTracingPipelineVK::addIntersectShaderGroup(const IntersectGroupParams& params)
{
	if (m_IntersectShaderGroups.size() > 0)
	{
		LOG("--- RayTracingPipeline: Failed to add Intersect Shader Group, currently a maximum of one shader group per type is allowed!");
		return;
	}
	
	VkRayTracingShaderGroupCreateInfoNV shaderGroupCreateInfo = {};
	shaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	shaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV; //TODO: What should these be?
	shaderGroupCreateInfo.generalShader = VK_SHADER_UNUSED_NV;
	
	if (params.pIntersectShader != nullptr)
	{
		m_Shaders.push_back(params.pIntersectShader);
		shaderGroupCreateInfo.intersectionShader = m_Shaders.size() - 1;
	}
	else
	{
		shaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_NV;
	}

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
		LOG("--- RayTracingPipeline: Failed to add Intersect Shader Group, Closest Hit Shader must be valid!");
		return;
	}
	
	m_IntersectShaderGroups.push_back(shaderGroupCreateInfo);
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

	for (auto& intersectShaderGroup : m_IntersectShaderGroups)
	{
		m_AllShaderGroups.push_back(intersectShaderGroup);
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
	
	VK_CHECK_RESULT_RETURN_FALSE(m_pDevice->vkCreateRayTracingPipelinesNV(m_pDevice->getDevice(), VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &m_Pipeline), "--- RayTracingPipeline: Failed to create RayTracingPipeline!");

	LOG("--- RayTracingPipeline: Successfully created RayTracingPipeline!");
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
	shaderStageInfo.pSpecializationInfo = nullptr;
}