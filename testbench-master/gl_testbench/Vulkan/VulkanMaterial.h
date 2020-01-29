#pragma once
#include "../Material.h"
#include "Common.h"

class VulkanDevice;

class VulkanMaterial : public Material
{
public:
	VulkanMaterial(VulkanDevice* pDevice, const std::string& name);
	~VulkanMaterial();

	DECL_NO_COPY(VulkanMaterial);

	virtual void setShader(const std::string& shaderFileName, ShaderType type);
	virtual void removeShader(ShaderType type);
	virtual void setDiffuse(Color c);
	virtual int compileMaterial(std::string& errString);
	virtual void addConstantBuffer(std::string name, unsigned int location);
	virtual void updateConstantBuffer(const void* data, size_t size, unsigned int location);
	virtual int enable();
	virtual void disable();

private:
	void deleteModule(VkShaderModule& module);
private:
	std::string m_Name;
	VulkanDevice* m_pDevice;
	VkShaderModule m_ShaderModules[4];
};

