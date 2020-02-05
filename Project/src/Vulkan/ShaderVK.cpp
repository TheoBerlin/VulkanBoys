#include "ShaderVK.h"

#include <fstream>

ShaderVK::ShaderVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_ShaderModule(VK_NULL_HANDLE),
	m_EntryPoint(),
	m_ShaderType(EShader::NONE)
{
}

ShaderVK::~ShaderVK()
{
	if (m_ShaderModule != VK_NULL_HANDLE)
	{
		//vkDestroyShaderModule(m_pDevice->getDevice(), m_ShaderModule, nullptr);
		m_ShaderModule = VK_NULL_HANDLE;

		std::cout << "Destroyed ShaderModule" << std::endl;
	}
}

bool ShaderVK::loadFromFile(EShader shaderType, const std::string& entrypoint, const std::string& filepath)
{
	std::ifstream shaderFile(filepath);
	if (shaderFile.is_open())
	{
		size_t fileSize = (size_t)shaderFile.tellg();
		m_Source.resize(fileSize);

		shaderFile.seekg(0);
		shaderFile.read(m_Source.data(), fileSize);
		shaderFile.close();

		m_EntryPoint = entrypoint;
		m_ShaderType = shaderType;

		std::cout << "Loaded shaderfile: " << filepath << " - Entrypoint: " << m_EntryPoint << std::endl;
		return true;
	}
	
	std::cerr << "Failed to load shaderfile: " << filepath << std::endl;
	return false;
}

bool ShaderVK::finalize()
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType	= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext	= nullptr;
	createInfo.flags	= 0;
	createInfo.codeSize = m_Source.size();
	createInfo.pCode	= reinterpret_cast<const uint32_t*>(m_Source.data());

	VkResult result = VK_SUCCESS;// = vkCreateShaderModule(m_pDevice->getDevice(), &createInfo, nullptr, &m_Module);
	if (result != VK_SUCCESS)
	{
		std::cout << "vkCreateShaderModule failed" << std::endl;
		return false;
	}
	else
	{
		std::cout << "Created ShaderModule" << std::endl;
		return true;
	}
}

const std::string& ShaderVK::getEntryPoint() const
{
	return m_EntryPoint;
}

EShader ShaderVK::getShaderType() const
{
	return m_ShaderType;
}
