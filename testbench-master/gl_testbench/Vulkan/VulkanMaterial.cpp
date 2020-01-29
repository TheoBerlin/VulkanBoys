#include "VulkanMaterial.h"

VulkanMaterial::VulkanMaterial()
{
}

VulkanMaterial::~VulkanMaterial()
{
}

void VulkanMaterial::setShader(const std::string& shaderFileName, ShaderType type)
{
}

void VulkanMaterial::removeShader(ShaderType type)
{
}

void VulkanMaterial::setDiffuse(Color c)
{
}

int VulkanMaterial::compileMaterial(std::string& errString)
{
	return 0;
}

void VulkanMaterial::addConstantBuffer(std::string name, unsigned int location)
{
}

void VulkanMaterial::updateConstantBuffer(const void* data, size_t size, unsigned int location)
{
}

int VulkanMaterial::enable()
{
	return 0;
}

void VulkanMaterial::disable()
{
}
