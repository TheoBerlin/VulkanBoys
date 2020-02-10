#pragma once

#include "Vulkan/VulkanCommon.h"

class PipelineLayoutVK;
class DeviceVK;
class ShaderVK;

struct RaygenGroupParams
{
	ShaderVK* pRaygenShader = nullptr;
};

struct IntersectGroupParams
{
	ShaderVK* pIntersectShader = nullptr;
	ShaderVK* pAnyHitShader = nullptr;
	ShaderVK* pClosestHitShader = nullptr;
};

struct MissGroupParams
{
	ShaderVK* pMissShader = nullptr;
};

class RayTracingPipelineVK
{
public:
	DECL_NO_COPY(RayTracingPipelineVK);

	RayTracingPipelineVK(DeviceVK* pDevice);
	~RayTracingPipelineVK();

	void addRaygenShaderGroup(const RaygenGroupParams& params);
	void addIntersectShaderGroup(const IntersectGroupParams& params);
	void addMissShaderGroup(const MissGroupParams& params);

	bool finalize(PipelineLayoutVK* pPipelineLayout);
	
	void setMaxRecursionDepth(uint32_t value) { m_MaxRecursionDepth = value; }

	VkPipeline getPipeline() { return m_Pipeline; }
	uint32_t getNumRaygenShaderGroups() { return m_RaygenShaderGroups.size(); }
	uint32_t getNumIntersectShaderGroups() { return m_IntersectShaderGroups.size(); }
	uint32_t getNumMissShaderGroups() { return m_MissShaderGroups.size(); }
	uint32_t getNumTotalShaderGroups() { return m_AllShaderGroups.size(); }
	
	uint32_t getNumShaders() { return m_Shaders.size(); }

private:
	void createShaderStageInfo(VkPipelineShaderStageCreateInfo& shaderStageInfo, const ShaderVK* pShader);
	
private:
	DeviceVK* m_pDevice;
	
	std::vector<VkRayTracingShaderGroupCreateInfoNV> m_RaygenShaderGroups;
	std::vector<VkRayTracingShaderGroupCreateInfoNV> m_IntersectShaderGroups;
	std::vector<VkRayTracingShaderGroupCreateInfoNV> m_MissShaderGroups;
	std::vector<VkRayTracingShaderGroupCreateInfoNV> m_AllShaderGroups; //Redundant
	std::vector<ShaderVK*> m_Shaders;

	uint32_t m_MaxRecursionDepth;

	VkPipeline m_Pipeline;
};
