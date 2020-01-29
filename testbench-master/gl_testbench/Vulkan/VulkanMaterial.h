#pragma once
#include "../Material.h"
#include "Common.h"

class VulkanMaterial : public Material
{
public:
	VulkanMaterial();
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

};

