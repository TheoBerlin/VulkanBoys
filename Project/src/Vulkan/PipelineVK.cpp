#include "PipelineVK.h"

#include "DeviceVK.h"
#include "PipelineLayoutVK.h"
#include "RenderPassVK.h"
#include "ShaderVK.h"

PipelineVK::PipelineVK(DeviceVK* pDevice)
	: m_pDevice(pDevice),
    m_Pipeline(VK_NULL_HANDLE),
	m_WireFrame(false)
{}

PipelineVK::~PipelineVK()
{
	if (m_Pipeline != VK_NULL_HANDLE && m_pDevice != nullptr) {
		vkDestroyPipeline(m_pDevice->getDevice(), m_Pipeline, nullptr);
		m_Pipeline = VK_NULL_HANDLE;
	}
}

void PipelineVK::addVertexBinding(uint32_t binding, VkVertexInputRate inputRate, uint32_t stride)
{
    VkVertexInputBindingDescription vertexInputbinding = {};
    vertexInputbinding.binding      =  binding;
    vertexInputbinding.inputRate    = inputRate;
    vertexInputbinding.stride       = stride;

    m_VertexBindings.emplace_back(vertexInputbinding);
}

void PipelineVK::addVertexAttribute(uint32_t binding, VkFormat format, uint32_t location, uint32_t offset)
{
    VkVertexInputAttributeDescription vertexInputAttribute = {};
    vertexInputAttribute.binding    = binding;
    vertexInputAttribute.format     = format;
    vertexInputAttribute.location   = location;
    vertexInputAttribute.offset     = offset;

    m_VertexAttributes.emplace_back(vertexInputAttribute);
}

void PipelineVK::addColorBlendAttachment(bool blendEnable, VkColorComponentFlags colorWriteMask)
{
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = colorWriteMask;
    colorBlendAttachment.blendEnable    = blendEnable ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor    = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor    = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp           = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp           = VK_BLEND_OP_ADD;

    m_ColorBlendAttachments.emplace_back(colorBlendAttachment);
}

void PipelineVK::create(const std::vector<IShader*>& shaders, RenderPassVK* pRenderPass, PipelineLayoutVK* pPipelineLayout)
{
    // Define shader stage create infos
    std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfos;
    shaderStagesInfos.reserve(shaders.size());

    for (IShader* shader : shaders) 
    {
        VkPipelineShaderStageCreateInfo shaderStageInfo;
        createShaderStageInfo(shaderStageInfo, shader);
        shaderStagesInfos.push_back(shaderStageInfo);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.flags = 0;
    vertexInputInfo.pNext = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = uint32_t(m_VertexAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions    = (m_VertexAttributes.size() > 0) ? m_VertexAttributes.data() : nullptr;
    vertexInputInfo.vertexBindingDescriptionCount   = uint32_t(m_VertexBindings.size());
    vertexInputInfo.pVertexBindingDescriptions      = (m_VertexBindings.size() > 0) ? m_VertexBindings.data() : nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = nullptr;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = nullptr;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = m_WireFrame ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth    = 1.0f;
    rasterizer.cullMode     = VK_CULL_MODE_NONE;
    rasterizer.frontFace    = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable  = VK_FALSE;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.flags = 0;
    colorBlending.pNext = nullptr;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp       = VK_LOGIC_OP_COPY;
    colorBlending.pAttachments      = (m_ColorBlendAttachments.size() > 0) ? m_ColorBlendAttachments.data() : nullptr;
    colorBlending.attachmentCount   = uint32_t(m_ColorBlendAttachments.size());
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.flags = 0;
    dynamicState.pNext = nullptr;
    dynamicState.pDynamicStates     = dynamicStates;
    dynamicState.dynamicStateCount  = 2;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable    = VK_FALSE;
    depthStencil.depthWriteEnable   = VK_FALSE;
    depthStencil.depthCompareOp     = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable  = VK_FALSE;
    depthStencil.stencilTestEnable      = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.front  = {};
    depthStencil.back   = {};

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = 0;
    pipelineInfo.stageCount = uint32_t(shaderStagesInfos.size());
    pipelineInfo.pStages    = shaderStagesInfos.data();
    pipelineInfo.pVertexInputState      = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState    = &inputAssembly;
    pipelineInfo.pViewportState         = &viewportState;
    pipelineInfo.pRasterizationState    = &rasterizer;
    pipelineInfo.pMultisampleState      = &multisampling;
    pipelineInfo.pDepthStencilState     = &depthStencil;
    pipelineInfo.pColorBlendState       = &colorBlending;
    pipelineInfo.pDynamicState          = &dynamicState;
    pipelineInfo.renderPass = pRenderPass->getRenderPass();
    pipelineInfo.layout     = pPipelineLayout->getPipelineLayout();
    pipelineInfo.subpass    = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex  = -1;

    VkResult result = vkCreateGraphicsPipelines(m_pDevice->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);
    if (result != VK_SUCCESS) {
        D_LOG("vkCreateGraphicsPipelines failed");
    } else {
        D_LOG("--- Pipeline: Vulkan pipeline created successfully");

        m_VertexAttributes.clear();
        m_VertexBindings.clear();
    }
}

void PipelineVK::createShaderStageInfo(VkPipelineShaderStageCreateInfo& shaderStageInfo, const IShader* shader)
{
    shaderStageInfo = {};

    const ShaderVK* shaderVK  = reinterpret_cast<const ShaderVK*>(shader);
    shaderStageInfo.stage     = convertShaderType(shaderVK->getShaderType());
    shaderStageInfo.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.pNext     = nullptr;
    shaderStageInfo.flags     = 0;
    shaderStageInfo.stage     = convertShaderType(shader->getShaderType());
    shaderStageInfo.module    = shaderVK->getShaderModule();
    shaderStageInfo.pName     = shader->getEntryPoint().c_str();
    shaderStageInfo.pSpecializationInfo = nullptr;
}
