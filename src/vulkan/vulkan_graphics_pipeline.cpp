#include "vulkan_graphics_pipeline.h"
#include "vulkan_context.h"

VulkanGraphicsPipeline::VulkanGraphicsPipeline(std::string debugName)
    : m_DebugName(std::move(debugName))
{
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    if (m_Pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(VulkanContext::Get().Device(), m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
    }
}

void VulkanGraphicsPipeline::SetShaderStages(const std::vector <VkPipelineShaderStageCreateInfo> &shaderStages)
{
    m_ShaderStages = shaderStages;
}

void VulkanGraphicsPipeline::SetVertexInputState(const VkPipelineVertexInputStateCreateInfo &vertexInputState)
{
    m_VertexInputState = vertexInputState;
}

void VulkanGraphicsPipeline::SetInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo &inputAssemblyState)
{
    m_InputAssemblyState = inputAssemblyState;
}

void VulkanGraphicsPipeline::SetViewportState(const VkPipelineViewportStateCreateInfo &viewportState)
{
    m_ViewportState = viewportState;
}

void VulkanGraphicsPipeline::SetRasterizationState(const VkPipelineRasterizationStateCreateInfo &rasterizationState)
{
    m_RasterizationState = rasterizationState;
}

void VulkanGraphicsPipeline::SetMultisampleState(const VkPipelineMultisampleStateCreateInfo &multisampleState)
{
    m_MultisampleState = multisampleState;
}

void VulkanGraphicsPipeline::SetDepthStencilState(const VkPipelineDepthStencilStateCreateInfo &depthStencilState)
{
    m_DepthStencilState = depthStencilState;
}

void VulkanGraphicsPipeline::SetColorBlendState(const VkPipelineColorBlendStateCreateInfo &colorBlendState)
{
    m_ColorBlendState = colorBlendState;
}

void VulkanGraphicsPipeline::SetDynamicState(const VkPipelineDynamicStateCreateInfo &dynamicState)
{
    m_DynamicState = dynamicState;
}

void VulkanGraphicsPipeline::SetLayout(VkPipelineLayout layout)
{
    m_Layout = layout;
}

void VulkanGraphicsPipeline::SetRenderPass(VkRenderPass renderPass, uint32_t subpass)
{
    m_RenderPass = renderPass;
    m_Subpass = subpass;
}

void VulkanGraphicsPipeline::Build()
{
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(m_ShaderStages.size());
    pipelineInfo.pStages = m_ShaderStages.data();
    pipelineInfo.pVertexInputState = &m_VertexInputState;
    pipelineInfo.pInputAssemblyState = &m_InputAssemblyState;
    pipelineInfo.pViewportState = &m_ViewportState;
    pipelineInfo.pRasterizationState = &m_RasterizationState;
    pipelineInfo.pMultisampleState = &m_MultisampleState;
    pipelineInfo.pDepthStencilState = &m_DepthStencilState;
    pipelineInfo.pColorBlendState = &m_ColorBlendState;
    pipelineInfo.pDynamicState = &m_DynamicState;
    pipelineInfo.layout = m_Layout;
    pipelineInfo.renderPass = m_RenderPass;
    pipelineInfo.subpass = m_Subpass;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkResult result = vkCreateGraphicsPipelines(VulkanContext::Get().Device(), VK_NULL_HANDLE, 1, &pipelineInfo,
                                                nullptr, &m_Pipeline);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline: " + m_DebugName);
    }
}

VulkanGraphicsPipelineBuilder::VulkanGraphicsPipelineBuilder(std::string debugName)
    :m_DebugName(std::move(debugName))
{
    SetupDefaultStates();
}

void VulkanGraphicsPipelineBuilder::SetupDefaultStates()
{
    // Input Assembly
    m_InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_InputAssemblyState.primitiveRestartEnable = VK_FALSE;

    // Rasterization
    m_RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_RasterizationState.depthClampEnable = VK_FALSE;
    m_RasterizationState.rasterizerDiscardEnable = VK_FALSE;
    m_RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    m_RasterizationState.lineWidth = 1.0f;
    m_RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    m_RasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    m_RasterizationState.depthBiasEnable = VK_FALSE;

    // Multisampling
    m_MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_MultisampleState.sampleShadingEnable = VK_FALSE;
    m_MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth and Stencil
    m_DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_DepthStencilState.depthTestEnable = VK_TRUE;
    m_DepthStencilState.depthWriteEnable = VK_TRUE;
    m_DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    m_DepthStencilState.depthBoundsTestEnable = VK_FALSE;
    m_DepthStencilState.stencilTestEnable = VK_FALSE;

    // Color Blend
    m_ColorBlendAttachment.blendEnable = VK_FALSE;
    m_ColorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    m_ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_ColorBlendState.logicOpEnable = VK_FALSE;
    m_ColorBlendState.attachmentCount = 1;
    m_ColorBlendState.pAttachments = &m_ColorBlendAttachment;

    // Dynamic State
    m_DynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    m_DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_DynamicState.dynamicStateCount = static_cast<uint32_t>(m_DynamicStates.size());
    m_DynamicState.pDynamicStates = m_DynamicStates.data();
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetShaders(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
{
    m_VertexShader = std::make_unique<VulkanShader>(vertexShaderPath, ShaderType::Vertex);
    m_FragmentShader = std::make_unique<VulkanShader>(fragmentShaderPath, ShaderType::Fragment);

    m_VertexShader->Load();
    m_FragmentShader->Load();

    m_ShaderStages.clear();
    m_ShaderStages.resize(2);

    m_ShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_ShaderStages[0].stage = m_VertexShader->GetShaderStage();
    m_ShaderStages[0].module = m_VertexShader->GetShaderModule();
    m_ShaderStages[0].pName = "main";

    m_ShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_ShaderStages[1].stage = m_FragmentShader->GetShaderStage();
    m_ShaderStages[1].module = m_FragmentShader->GetShaderModule();
    m_ShaderStages[1].pName = "main";

    return *this;    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetPolygonMode(VkPolygonMode mode)
{
    m_RasterizationState.polygonMode = mode;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
    m_RasterizationState.cullMode = cullMode;
    m_RasterizationState.frontFace = frontFace;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetMultisampling(VkSampleCountFlagBits samples)
{
    m_MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_MultisampleState.sampleShadingEnable = VK_FALSE;
    m_MultisampleState.rasterizationSamples = samples;
    m_MultisampleState.minSampleShading = 1.0f;
    m_MultisampleState.pSampleMask = nullptr;
    m_MultisampleState.alphaToCoverageEnable = VK_FALSE;
    m_MultisampleState.alphaToOneEnable = VK_FALSE;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetDepthTesting(bool enable, bool writeEnable, VkCompareOp compareOp)
{
    m_DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_DepthStencilState.depthTestEnable = enable ? VK_TRUE : VK_FALSE;
    m_DepthStencilState.depthWriteEnable = writeEnable ? VK_TRUE : VK_FALSE;
    m_DepthStencilState.depthCompareOp = compareOp;
    m_DepthStencilState.depthBoundsTestEnable = VK_FALSE;
    m_DepthStencilState.stencilTestEnable = VK_FALSE;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetColorBlendAttachment(bool blendEnable, VkColorComponentFlags colorWriteMask)
{
    m_ColorBlendAttachment.blendEnable = blendEnable ? VK_TRUE : VK_FALSE;
    m_ColorBlendAttachment.colorWriteMask = colorWriteMask;

    if (blendEnable)
    {
        m_ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        m_ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        m_ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        m_ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        m_ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        m_ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    m_ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_ColorBlendState.logicOpEnable = VK_FALSE;
    m_ColorBlendState.logicOp = VK_LOGIC_OP_COPY;
    m_ColorBlendState.attachmentCount = 1;
    m_ColorBlendState.pAttachments = &m_ColorBlendAttachment;
    m_ColorBlendState.blendConstants[0] = 0.0f;
    m_ColorBlendState.blendConstants[1] = 0.0f;
    m_ColorBlendState.blendConstants[2] = 0.0f;
    m_ColorBlendState.blendConstants[3] = 0.0f;

    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetDynamicStates(const std::vector<VkDynamicState> &dynamicStates)
{
    m_DynamicStates = dynamicStates;
    m_DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_DynamicState.dynamicStateCount = static_cast<uint32_t>(m_DynamicStates.size());
    m_DynamicState.pDynamicStates = m_DynamicStates.data();
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetLayout(VkPipelineLayout layout)
{
    m_Pipeline.SetLayout(layout);
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetRenderPass(VkRenderPass renderPass, uint32_t subpass)
{
    m_Pipeline.SetRenderPass(renderPass, subpass);
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetVertexInputDescription(const VertexInputDescription &description)
{
    m_VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    m_VertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(description.bindings.size());
    m_VertexInputState.pVertexBindingDescriptions = description.bindings.data();
    m_VertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(description.attributes.size());
    m_VertexInputState.pVertexAttributeDescriptions = description.attributes.data();
    return *this;
}

VulkanGraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetPrimitiveTopology(VkPrimitiveTopology topology)
{
    m_InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_InputAssemblyState.topology = topology;
    m_InputAssemblyState.primitiveRestartEnable = VK_FALSE;
    return *this;
}

std::unique_ptr<VulkanGraphicsPipeline> VulkanGraphicsPipelineBuilder::Build()
{
    auto pipeline = std::make_unique<VulkanGraphicsPipeline>(m_DebugName);
    pipeline->SetShaderStages(m_ShaderStages);
    pipeline->SetVertexInputState(m_VertexInputState);
    pipeline->SetInputAssemblyState(m_InputAssemblyState);
    pipeline->SetViewportState(m_ViewportState);
    pipeline->SetRasterizationState(m_RasterizationState);
    pipeline->SetMultisampleState(m_MultisampleState);
    pipeline->SetDepthStencilState(m_DepthStencilState);
    pipeline->SetColorBlendState(m_ColorBlendState);
    pipeline->SetDynamicState(m_DynamicState);
    pipeline->Build();
    return pipeline;
}