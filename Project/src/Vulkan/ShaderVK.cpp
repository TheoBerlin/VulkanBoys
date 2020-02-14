#include "ShaderVK.h"
#include "DeviceVK.h"

#include <fstream>

ShaderVK::ShaderVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
	m_ShaderModule(VK_NULL_HANDLE),
	m_EntryPoint(),
	m_ShaderType(EShader::NONE),
	m_SpecializationInfo({})
{
}

ShaderVK::~ShaderVK()
{
	if (m_ShaderModule != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(m_pDevice->getDevice(), m_ShaderModule, nullptr);
		m_ShaderModule = VK_NULL_HANDLE;

		D_LOG("Destroyed ShaderModule");
	}
}

bool ShaderVK::initFromFile(EShader shaderType, const std::string& entrypoint, const std::string& filepath)
{
	std::ifstream shaderFile(filepath, std::ios::ate | std::ios::binary);
	if (shaderFile.is_open())
	{
		size_t fileSize = (size_t)shaderFile.tellg();
		std::vector<char> byteCode(fileSize);

		shaderFile.seekg(0);
		shaderFile.read(byteCode.data(), fileSize);
		shaderFile.close();

		D_LOG("Loaded shaderfile: %s - Entrypoint: %s", filepath.c_str(), entrypoint.c_str());
		return initFromByteCode(shaderType, entrypoint, byteCode);
	}
	
	LOG("Failed to load shaderfile: %s", filepath.c_str());
	return false;
}

bool ShaderVK::initFromByteCode(EShader shaderType, const std::string& entrypoint, const std::vector<char>& byteCode)
{
	m_EntryPoint = entrypoint;
	m_ShaderType = shaderType;
	m_Source = byteCode;
	return true;
}

bool ShaderVK::finalize()
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType	= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext	= nullptr;
	createInfo.flags	= 0;
	createInfo.codeSize = m_Source.size();
	createInfo.pCode	= reinterpret_cast<const uint32_t*>(m_Source.data());

	VkResult result = vkCreateShaderModule(m_pDevice->getDevice(), &createInfo, nullptr, &m_ShaderModule);
	if (result != VK_SUCCESS)
	{
		LOG("vkCreateShaderModule failed");
		return false;
	}
	else
	{
		D_LOG("--- ShaderModule: Vulkan ShaderModule created successfully");
		return true;
	}
}

const std::string& ShaderVK::getEntryPoint() const
{
	return m_EntryPoint;
}

void ShaderVK::setSpecializationConstant(uint32_t index, void* pData, uint32_t sizeInBytes)
{
	uint32_t specializationDataOffset = m_SpecializationData.size();
	m_SpecializationData.resize(specializationDataOffset + sizeInBytes);
	memcpy(m_SpecializationData.data(), pData, sizeInBytes);

	VkSpecializationMapEntry specializationEntry = {};
	specializationEntry.constantID = index;
	specializationEntry.offset = 0;
	specializationEntry.size = sizeInBytes;
	m_SpecializationEntries.push_back(specializationEntry);

	m_SpecializationInfo.mapEntryCount = m_SpecializationEntries.size();
	m_SpecializationInfo.pMapEntries = m_SpecializationEntries.data();
	m_SpecializationInfo.dataSize = m_SpecializationData.size();
	m_SpecializationInfo.pData = m_SpecializationData.data();
}

EShader ShaderVK::getShaderType() const
{
	return m_ShaderType;
}
