#include "VulkanMaterial.h"
#include "VulkanDevice.h"
#include "VulkanConstantBuffer.h"
#include "VulkanRenderer.h"
#include "../IA.h"

#include <fstream>

VulkanMaterial::VulkanMaterial(VulkanRenderer* pRenderer, VulkanDevice* pDevice, const std::string& name)
	: Material(),
	m_pRenderer(pRenderer),
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
	vkDeviceWaitIdle(m_pDevice->getDevice());
	
	constexpr uint32_t shaderCount = sizeof(m_ShaderModules) / sizeof(VkShaderModule);
	for (uint32_t i = 0; i < shaderCount; i++)
		deleteModule(m_ShaderModules[i]);

	for (auto buffer : m_ConstantBuffers)
		delete buffer.second;
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
	if (constructShader(ShaderType::VS, errString) < 0)
		return -1;

	if (constructShader(ShaderType::PS, errString) < 0)
		return -1;

	return 0;
}

void VulkanMaterial::addConstantBuffer(std::string name, unsigned int location)
{
	m_ConstantBuffers[location] = reinterpret_cast<VulkanConstantBuffer*>(m_pRenderer->makeConstantBuffer(name, location));
}

void VulkanMaterial::updateConstantBuffer(const void* data, size_t size, unsigned int location)
{
	m_ConstantBuffers[location]->setData(data, size, this, location);
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

int32_t VulkanMaterial::constructShader(ShaderType type, std::string& errString)
{
	//Construct a shader from the shaderfile
	std::string shader = "#version 450\n#extension GL_ARB_separate_shader_objects : enable\n\n";
	std::set<std::string> defines = shaderDefines[type];
	for (std::string define : defines)
		shader += define;
	shader += "\n";

	std::string& filename = shaderFileNames[type];
	std::ifstream shaderFile(filename);
	if (shaderFile.is_open()) 
	{
		shader += std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
		shaderFile.close();
	}
	else 
	{
		errString = "Cannot find file: " + filename;
		return -1;
	}

	//write the new shaderfile to disk
	std::string tmpFileName = std::string(filename.begin(), filename.begin() + filename.find_last_of('.')) + "_tmp";
	std::string tmpFileNameGLSL = tmpFileName + ".glsl";
	std::ofstream tmpShaderFile(tmpFileNameGLSL);
	if (tmpShaderFile.is_open())
	{
		//std::cout << shader << std::endl;

		tmpShaderFile << shader;
		tmpShaderFile.close();
	}
	else
	{
		errString = "Cannot find file: " + tmpFileNameGLSL;
		return -1;
	}

	std::string stage;
	if (type == ShaderType::VS)
		stage = "vertex";
	else if (type == ShaderType::GS)
		stage = "geometry";
	else if (type == ShaderType::CS)
		stage = "compute";
	else if (type == ShaderType::PS)
		stage = "fragment";

	//Compile the shaderfile on disk
	std::string tmpFileNameSPIRV = tmpFileName + ".spv";
	std::string command = "..\\tools\\glslc -fshader-stage=" + stage + " " + tmpFileNameGLSL + " -o " + tmpFileNameSPIRV;
	int result = std::system(command.c_str());
	if (result != 0)
	{
		errString = "Failed to compile " + tmpFileNameGLSL;
		return -1;
	}

	//Create module
	std::ifstream compiledFile(tmpFileNameSPIRV, std::ios::ate | std::ios::binary);
	if (!compiledFile.is_open()) 
	{
		errString = "Cannot find file: " + tmpFileNameSPIRV;
		return -1;
	}

	size_t fileSize = (size_t)compiledFile.tellg();
	std::vector<char> code(fileSize);
	compiledFile.seekg(0);
	compiledFile.read(code.data(), fileSize);
	compiledFile.close();

	VkShaderModuleCreateInfo shaderInfo = {};
	shaderInfo.sType	= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.pNext	= nullptr;
	shaderInfo.flags	= 0;
	shaderInfo.codeSize = code.size();
	shaderInfo.pCode	= (const uint32_t*)(code.data());

	VkResult res = vkCreateShaderModule(m_pDevice->getDevice(), &shaderInfo, nullptr, &m_ShaderModules[(uint32_t)type]);
	if (res != VK_SUCCESS) 
	{
		errString = "vkCreateShaderModule failed";
		return -1;
	}
	else
	{
		std::cout << "Created ShaderModule" << std::endl;
	}

	return 0;
}
