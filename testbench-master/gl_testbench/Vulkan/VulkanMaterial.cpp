#include "VulkanMaterial.h"
#include "VulkanDevice.h"
#include "VulkanConstantBuffer.h"
#include "VulkanRenderer.h"

#include <fstream>

VulkanMaterial::VulkanMaterial(VulkanRenderer* pRenderer, VulkanDevice* pDevice, const std::string& name)
	: Material(),
	m_pRenderer(pRenderer),
	m_Name(name),
	m_pDevice(pDevice),
	m_ShaderModules(),
	m_PipelineLayout(VK_NULL_HANDLE),
	m_DescriptorSetLayout(VK_NULL_HANDLE)
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

	for (auto buffer : m_ConstantBuffers)
		delete buffer.second;
    
	if (m_DescriptorSetLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(m_pDevice->getDevice(), m_DescriptorSetLayout, nullptr);
		m_DescriptorSetLayout = VK_NULL_HANDLE;
	}
	
	if (m_PipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(m_pDevice->getDevice(), m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}
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
	//createDescriptorSetLayout();
	//createPipelineLayout();
	//createDescriptorSets();
	if (constructShader(ShaderType::VS, errString) < 0)
		return -1;

	if (constructShader(ShaderType::PS, errString) < 0)
		return -1;

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

void VulkanMaterial::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding bindings[] = { uboLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(m_pDevice->getDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Descriptor Set Layout!");
	}
}

void VulkanMaterial::createPipelineLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(m_pDevice->getDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to create Pipeline Layout!");
	}
}

void VulkanMaterial::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(DESCRIPTOR_SETS_PER_MATERIAL, m_DescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_pDevice->getDescriptorPool();
	allocInfo.descriptorSetCount = DESCRIPTOR_SETS_PER_MATERIAL;
	allocInfo.pSetLayouts = layouts.data();

	if (vkAllocateDescriptorSets(m_pDevice->getDevice(), &allocInfo, m_DescriptorSets) != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to allocate Descriptor Sets!");
	}

	for (size_t i = 0; i < DESCRIPTOR_SETS_PER_MATERIAL; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = VK_NULL_HANDLE; // uniformBuffers[i]; //<-- Theo Jobba
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet descriptorBufferWrite = {};
		descriptorBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorBufferWrite.dstSet = m_DescriptorSets[i];
		descriptorBufferWrite.dstBinding = 0;
		descriptorBufferWrite.dstArrayElement = 0;
		descriptorBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorBufferWrite.descriptorCount = UNIFORM_DESCRIPTORS_PER_SET;
		descriptorBufferWrite.pBufferInfo = &bufferInfo;
		descriptorBufferWrite.pImageInfo = nullptr;
		descriptorBufferWrite.pTexelBufferView = nullptr;

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = nullptr;
		imageInfo.sampler = nullptr;

		VkWriteDescriptorSet descriptorImageWrite = {};
		descriptorImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorImageWrite.dstSet = m_DescriptorSets[i];
		descriptorImageWrite.dstBinding = 1;
		descriptorImageWrite.dstArrayElement = 0;
		descriptorImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorImageWrite.descriptorCount = SAMPLER_DESCRIPTORS_PER_SET;
		descriptorImageWrite.pBufferInfo = nullptr;
		descriptorImageWrite.pImageInfo = &imageInfo;
		descriptorImageWrite.pTexelBufferView = nullptr;

		VkWriteDescriptorSet descriptorWrites[] = { descriptorBufferWrite, descriptorImageWrite };
		vkUpdateDescriptorSets(m_pDevice->getDevice(), 2, descriptorWrites, 0, nullptr);
	}
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
