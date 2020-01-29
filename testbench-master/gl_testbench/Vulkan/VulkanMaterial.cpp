#include "VulkanMaterial.h"
#include "VulkanDevice.h"

VulkanMaterial::VulkanMaterial(VulkanDevice* pDevice, const std::string& name)
	: Material(),
	m_Name(name),
	m_pDevice(pDevice),
	m_ShaderModules()
{
	isValid = false;

	//memset to zero?
	constexpr uint32_t shaderCount = sizeof(m_ShaderModules) / sizeof(VkShaderModule);
	for (uint32_t i = 0; i < shaderCount; i++)
		m_ShaderModules[i] = VK_NULL_HANDLE;
}

VulkanMaterial::~VulkanMaterial()
{
	constexpr uint32_t shaderCount = sizeof(m_ShaderModules) / sizeof(VkShaderModule);
	for (uint32_t i = 0; i < shaderCount; i++)
		deleteModule(m_ShaderModules[i]);
}

void VulkanMaterial::setShader(const std::string& shaderFileName, ShaderType type)
{
	if (shaderFileNames.find(type) != shaderFileNames.end())
	{
		removeShader(type);
	}
	shaderFileNames[type] = shaderFileName;
}

void VulkanMaterial::removeShader(ShaderType type)
{
	VkShaderModule module = m_ShaderModules[(uint32_t)type];
	deleteModule(module);
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

void VulkanMaterial::deleteModule(VkShaderModule& module)
{
	if (module != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(m_pDevice->getDevice(), module, nullptr);
		module = VK_NULL_HANDLE;
	}
}
