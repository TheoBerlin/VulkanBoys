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
	createDescriptorSetLayout();
	createPipelineLayout();
	createDescriptorSets();
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
		bufferInfo.buffer = uniformBuffers[i]; //<-- Theo Jobba
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
