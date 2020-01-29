#pragma once

class VulkanConstantBuffer;
class VulkanRenderer;

#include "../Material.h"
#include "Common.h"

class VulkanDevice;

class VulkanMaterial : public Material
{
public:
	VulkanMaterial(VulkanRenderer* pRenderer, VulkanDevice* pDevice, const std::string& name);
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
	void createDescriptorSetLayout();
	void createPipelineLayout();
	void createDescriptorSets();
	
private:
	VulkanRenderer* m_pRenderer;

	std::map<unsigned int, VulkanConstantBuffer*> m_ConstantBuffers;

	std::string m_Name;
	VulkanDevice* m_pDevice;
	VkShaderModule m_ShaderModules[4];

	VkPipelineLayout m_PipelineLayout;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkDescriptorSet m_DescriptorSets[DESCRIPTOR_SETS_PER_MATERIAL];
};

