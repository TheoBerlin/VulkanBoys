#include "PipelineVK.h"

#include "DeviceVK.h"
#include "PipelineLayoutVK.h"
#include "RenderPassVK.h"
#include "ShaderVK.h"

PipelineVK::PipelineVK(DeviceVK* pDevice)
	: m_VertexBindings(),
    m_VertexAttributes(),
    m_ColorBlendAttachments(),
    m_InputAssembly(),
    m_RasterizerState(),
    m_MultisamplingState(),
    m_BlendState(),
    m_DepthStencilState(),
    m_pDevice(pDevice),
    m_Pipeline(VK_NULL_HANDLE)
{
    //Default InputAssembly
    m_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_InputAssembly.flags = 0;
    m_InputAssembly.pNext = nullptr;
    m_InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_InputAssembly.primitiveRestartEnable = VK_FALSE;

    //Default RasterizerState
    m_RasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_RasterizerState.flags = 0;
    m_RasterizerState.pNext = nullptr;
    m_RasterizerState.polygonMode  = VK_POLYGON_MODE_FILL;
    m_RasterizerState.lineWidth    = 1.0f;
    m_RasterizerState.cullMode     = VK_CULL_MODE_NONE;
    m_RasterizerState.frontFace    = VK_FRONT_FACE_CLOCKWISE;
    m_RasterizerState.depthBiasEnable  = VK_FALSE;
    m_RasterizerState.depthClampEnable = VK_FALSE;
    m_RasterizerState.rasterizerDiscardEnable = VK_FALSE;
    m_RasterizerState.depthBiasClamp = 0.0f;
    m_RasterizerState.depthBiasConstantFactor = 0.0f;
    m_RasterizerState.depthBiasSlopeFactor = 0.0f;

    //MultisamplingState
    m_MultisamplingState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_MultisamplingState.flags = 0;
    m_MultisamplingState.pNext = nullptr;
    m_MultisamplingState.alphaToCoverageEnable = VK_FALSE;
    m_MultisamplingState.alphaToOneEnable = VK_FALSE;
    m_MultisamplingState.minSampleShading = 0.0f;
    m_MultisamplingState.sampleShadingEnable = VK_FALSE;
    m_MultisamplingState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_MultisamplingState.pSampleMask = nullptr;

    //BlendState
    m_BlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_BlendState.flags = 0;
    m_BlendState.pNext = nullptr;
    m_BlendState.logicOpEnable = VK_FALSE;
    m_BlendState.logicOp = VK_LOGIC_OP_COPY;
    m_BlendState.pAttachments = nullptr;
    m_BlendState.attachmentCount = 0;
    m_BlendState.blendConstants[0] = 0.0f;
    m_BlendState.blendConstants[1] = 0.0f;
    m_BlendState.blendConstants[2] = 0.0f;
    m_BlendState.blendConstants[3] = 0.0f;

    //DepthstencilState
    m_DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_DepthStencilState.flags = 0;
    m_DepthStencilState.pNext = nullptr;
    m_DepthStencilState.depthTestEnable = VK_FALSE;
    m_DepthStencilState.depthWriteEnable = VK_TRUE;
    m_DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    m_DepthStencilState.depthBoundsTestEnable = VK_FALSE;
    m_DepthStencilState.stencilTestEnable = VK_FALSE;
    m_DepthStencilState.minDepthBounds = 0.0f;
    m_DepthStencilState.maxDepthBounds = 1.0f;
    m_DepthStencilState.front = {};
    m_DepthStencilState.back = {};
}

PipelineVK::~PipelineVK()
{
	if (m_Pipeline != VK_NULL_HANDLE && m_pDevice != nullptr) 
    {
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

void PipelineVK::addColorBlendAttachment(const VkPipelineColorBlendAttachmentState& colorBlendAttachment)
{
    m_ColorBlendAttachments.emplace_back(colorBlendAttachment);
}

void PipelineVK::setInputAssembly(VkPrimitiveTopology topology, bool primitiveRestartEnable)
{
    m_InputAssembly.topology                = topology;
    m_InputAssembly.primitiveRestartEnable  = primitiveRestartEnable ? VK_TRUE : VK_FALSE;
}

void PipelineVK::setRasterizerState(const VkPipelineRasterizationStateCreateInfo& rasterizerState)
{
    m_RasterizerState       = rasterizerState;
    m_RasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
}

void PipelineVK::setDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& depthStencilState)
{
    m_DepthStencilState         = depthStencilState;
    m_DepthStencilState.sType   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
}

void PipelineVK::setBlendState(VkLogicOp logicOp, bool logicOpEnable, float blendConstants[4])
{
    m_BlendState.logicOp        = logicOp;
    m_BlendState.logicOpEnable  = logicOpEnable ? VK_TRUE : VK_FALSE;
    
    memcpy(m_BlendState.blendConstants, blendConstants, sizeof(float) * 4);
}

bool PipelineVK::finalize(const std::vector<IShader*>& shaders, RenderPassVK* pRenderPass, PipelineLayoutVK* pPipelineLayout)
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
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.flags                           = 0;
    vertexInputInfo.pNext                           = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = uint32_t(m_VertexAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions    = (m_VertexAttributes.size() > 0) ? m_VertexAttributes.data() : nullptr;
    vertexInputInfo.vertexBindingDescriptionCount   = uint32_t(m_VertexBindings.size());
    vertexInputInfo.pVertexBindingDescriptions      = (m_VertexBindings.size() > 0) ? m_VertexBindings.data() : nullptr;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.flags         = 0;
    viewportState.pNext         = nullptr;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = nullptr;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = nullptr;

    VkDynamicState dynamicStates[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.flags              = 0;
    dynamicState.pNext              = nullptr;
    dynamicState.pDynamicStates     = dynamicStates;
    dynamicState.dynamicStateCount  = 2;

    m_BlendState.pAttachments       = m_ColorBlendAttachments.data();
    m_BlendState.attachmentCount    = uint32_t(m_ColorBlendAttachments.size());

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext                  = nullptr;
    pipelineInfo.flags                  = 0;
    pipelineInfo.stageCount             = uint32_t(shaderStagesInfos.size());
    pipelineInfo.pStages                = shaderStagesInfos.data();
    pipelineInfo.pVertexInputState      = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState    = &m_InputAssembly;
    pipelineInfo.pViewportState         = &viewportState;
    pipelineInfo.pRasterizationState    = &m_RasterizerState;
    pipelineInfo.pMultisampleState      = &m_MultisamplingState;
    pipelineInfo.pDepthStencilState     = &m_DepthStencilState;
    pipelineInfo.pColorBlendState       = &m_BlendState;
    pipelineInfo.pDynamicState          = &dynamicState;
    pipelineInfo.renderPass             = pRenderPass->getRenderPass();
    pipelineInfo.layout                 = pPipelineLayout->getPipelineLayout();
    pipelineInfo.subpass                = 0;
    pipelineInfo.basePipelineHandle     = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex      = -1;

    VK_CHECK_RESULT_RETURN_FALSE(vkCreateGraphicsPipelines(m_pDevice->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline), "vkCreateGraphicsPipelines failed");

    m_BindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    D_LOG("--- Pipeline: Vulkan graphics pipeline created successfully");

    m_VertexAttributes.clear();
    m_VertexBindings.clear();

    return true;
}

bool PipelineVK::finalizeCompute(IShader* shader, PipelineLayoutVK* pPipelineLayout)
{
    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = 0;
    createShaderStageInfo(pipelineInfo.stage, shader);
    pipelineInfo.layout = pPipelineLayout->getPipelineLayout();
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VK_CHECK_RESULT_RETURN_FALSE(vkCreateComputePipelines(m_pDevice->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline), "vkCreateComputePipelines failed");

    m_BindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
    D_LOG("--- Pipeline: Vulkan graphics pipeline created successfully");
    return true;
}

void PipelineVK::createShaderStageInfo(VkPipelineShaderStageCreateInfo& shaderStageInfo, const IShader* shader)
{
    shaderStageInfo = {};

    const ShaderVK* shaderVK  = reinterpret_cast<const ShaderVK*>(shader);
    shaderStageInfo.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.pNext     = nullptr;
    shaderStageInfo.flags     = 0;
    shaderStageInfo.stage     = convertShaderType(shader->getShaderType());
    shaderStageInfo.module    = shaderVK->getShaderModule();
    shaderStageInfo.pName     = shader->getEntryPoint().c_str();
    shaderStageInfo.pSpecializationInfo = reinterpret_cast<const ShaderVK*>(shader)->getSpecializationInfo();
}
