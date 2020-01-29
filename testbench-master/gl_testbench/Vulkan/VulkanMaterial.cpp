#include "VulkanMaterial.h"

#include "VulkanConstantBuffer.h"
#include "VulkanRenderer.h"

VulkanMaterial::VulkanMaterial(VulkanRenderer* renderer)
	:m_pRenderer(renderer)
{}

VulkanMaterial::~VulkanMaterial()
{
	for (auto buffer : m_ConstantBuffers) {
		delete buffer.second;
	}
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
	VulkanConstantBuffer* pConstantBuffer = new VulkanConstantBuffer(name, location);
	pConstantBuffer->provideResources(m_pRenderer);
	m_ConstantBuffers[location] = pConstantBuffer;
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
