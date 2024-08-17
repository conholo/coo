// vulkan_graphics_pipeline.cpp

#include "vulkan_graphics_pipeline.h"

#include "vulkan_context.h"
#include "vulkan_utils.h"

#include <iostream>
#include <stdexcept>

VulkanGraphicsPipeline::VulkanGraphicsPipeline(std::string debugName)
        : m_DebugName(std::move(debugName))
{}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    if (m_Pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(VulkanContext::Get().Device(), m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
    }
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other) noexcept
        : m_DebugName(std::move(other.m_DebugName)),
          m_Pipeline(other.m_Pipeline),
          m_Layout(other.m_Layout),
          m_RenderPass(other.m_RenderPass),
          m_Subpass(other.m_Subpass),
          m_ShaderStages(std::move(other.m_ShaderStages)),
          m_VertexInputState(other.m_VertexInputState),
          m_BindingDescriptions(std::move(other.m_BindingDescriptions)),
          m_AttributeDescriptions(std::move(other.m_AttributeDescriptions)),
          m_InputAssemblyState(other.m_InputAssemblyState),
          m_ViewportState(other.m_ViewportState),
          m_RasterizationState(other.m_RasterizationState),
          m_MultisampleState(other.m_MultisampleState),
          m_DepthStencilState(other.m_DepthStencilState),
          m_ColorBlendState(other.m_ColorBlendState),
          m_ColorBlendAttachments(std::move(other.m_ColorBlendAttachments)),
          m_DynamicState(other.m_DynamicState),
          m_DynamicStates(std::move(other.m_DynamicStates))
{
    other.m_Pipeline = VK_NULL_HANDLE;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::operator=(VulkanGraphicsPipeline&& other) noexcept
{
    if (this != &other)
    {
        if (m_Pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(VulkanContext::Get().Device(), m_Pipeline, nullptr);
        }

        m_DebugName = std::move(other.m_DebugName);
        m_Pipeline = other.m_Pipeline;
        m_Layout = other.m_Layout;
        m_RenderPass = other.m_RenderPass;
        m_Subpass = other.m_Subpass;
        m_ShaderStages = std::move(other.m_ShaderStages);
        m_VertexInputState = other.m_VertexInputState;
        m_BindingDescriptions = std::move(other.m_BindingDescriptions);
        m_AttributeDescriptions = std::move(other.m_AttributeDescriptions);
        m_InputAssemblyState = other.m_InputAssemblyState;
        m_ViewportState = other.m_ViewportState;
        m_RasterizationState = other.m_RasterizationState;
        m_MultisampleState = other.m_MultisampleState;
        m_DepthStencilState = other.m_DepthStencilState;
        m_ColorBlendState = other.m_ColorBlendState;
        m_ColorBlendAttachments = std::move(other.m_ColorBlendAttachments);
        m_DynamicState = other.m_DynamicState;
        m_DynamicStates = std::move(other.m_DynamicStates);

        other.m_Pipeline = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanGraphicsPipeline::SetShaderStages(std::vector<VkPipelineShaderStageCreateInfo>&& shaderStages)
{
    m_ShaderStages = std::move(shaderStages);
}

void VulkanGraphicsPipeline::SetVertexInputState(const VkPipelineVertexInputStateCreateInfo& vertexInputState,
                                                 std::vector<VkVertexInputBindingDescription>&& bindings,
                                                 std::vector<VkVertexInputAttributeDescription>&& attributes)
{
    m_VertexInputState = vertexInputState;
    m_BindingDescriptions = std::move(bindings);
    m_AttributeDescriptions = std::move(attributes);
    m_VertexInputState.pVertexBindingDescriptions = m_BindingDescriptions.data();
    m_VertexInputState.pVertexAttributeDescriptions = m_AttributeDescriptions.data();
}

void VulkanGraphicsPipeline::SetInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo& inputAssemblyState)
{
    m_InputAssemblyState = inputAssemblyState;
}

void VulkanGraphicsPipeline::SetViewportState(const VkPipelineViewportStateCreateInfo& viewportState)
{
    m_ViewportState = viewportState;
}

void VulkanGraphicsPipeline::SetRasterizationState(const VkPipelineRasterizationStateCreateInfo& rasterizationState)
{
    m_RasterizationState = rasterizationState;
}

void VulkanGraphicsPipeline::SetMultisampleState(const VkPipelineMultisampleStateCreateInfo& multisampleState)
{
    m_MultisampleState = multisampleState;
}

void VulkanGraphicsPipeline::SetDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& depthStencilState)
{
    m_DepthStencilState = depthStencilState;
}

void VulkanGraphicsPipeline::SetColorBlendState(const VkPipelineColorBlendStateCreateInfo& colorBlendState,
                                                std::vector<VkPipelineColorBlendAttachmentState>&& colorBlendAttachments)
{
    m_ColorBlendState = colorBlendState;
    m_ColorBlendAttachments = std::move(colorBlendAttachments);
    m_ColorBlendState.pAttachments = m_ColorBlendAttachments.data();
}

void VulkanGraphicsPipeline::SetDynamicState(const VkPipelineDynamicStateCreateInfo& dynamicState,
                                             std::vector<VkDynamicState>&& dynamicStates)
{
    m_DynamicState = dynamicState;
    m_DynamicStates = std::move(dynamicStates);
    m_DynamicState.pDynamicStates = m_DynamicStates.data();
	m_DynamicState.dynamicStateCount = static_cast<uint32_t>(m_DynamicStates.size());
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

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(VulkanContext::Get().Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline));
	SetDebugUtilsObjectName(VulkanContext::Get().Device(), VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_Pipeline, m_DebugName.c_str());
}

VulkanGraphicsPipelineBuilder::VulkanGraphicsPipelineBuilder(std::string debugName)
        : m_DebugName(std::move(debugName))
{
    SetupDefaultStates();
}

void VulkanGraphicsPipelineBuilder::SetupDefaultStates()
{
    m_InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_InputAssemblyState.primitiveRestartEnable = VK_FALSE;

    m_RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_RasterizationState.depthClampEnable = VK_FALSE;
    m_RasterizationState.rasterizerDiscardEnable = VK_FALSE;
    m_RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    m_RasterizationState.lineWidth = 1.0f;
    m_RasterizationState.cullMode = VK_CULL_MODE_NONE;
    m_RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    m_RasterizationState.depthBiasEnable = VK_FALSE;

    m_MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	m_MultisampleState.pSampleMask = nullptr;

	// Depth and stencil state containing depth and stencil compare and test operations
	// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
    m_DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_DepthStencilState.depthTestEnable = VK_TRUE;
    m_DepthStencilState.depthWriteEnable = VK_TRUE;
    m_DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    m_DepthStencilState.depthBoundsTestEnable = VK_FALSE;
    m_DepthStencilState.stencilTestEnable = VK_FALSE;

    m_ColorBlendAttachmentStates.resize(1);
    m_ColorBlendAttachmentStates[0].blendEnable = VK_FALSE;
    m_ColorBlendAttachmentStates[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    m_ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_ColorBlendState.attachmentCount = 1;
    m_ColorBlendState.pAttachments = m_ColorBlendAttachmentStates.data();

	// Viewport state sets the number of viewports and scissor used in this pipeline
	// Overridden by the dynamic states
    m_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_ViewportState.viewportCount = 1;
    m_ViewportState.scissorCount = 1;

	// Enable dynamic states
	// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
	// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
	// For this example we will set the viewport and scissor using dynamic states
    m_DynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    m_DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_DynamicState.dynamicStateCount = static_cast<uint32_t>(m_DynamicStates.size());
    m_DynamicState.pDynamicStates = m_DynamicStates.data();
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetShaders(VulkanShader& vertexShader, VulkanShader& fragmentShader)
{
    m_ShaderStages.resize(2);
    m_ShaderStages[0] = {};
    m_ShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_ShaderStages[0].stage = vertexShader.GetShaderStage();
    m_ShaderStages[0].module = vertexShader.GetShaderModule();
    m_ShaderStages[0].pName = "main";

    m_ShaderStages[1] = {};
    m_ShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    m_ShaderStages[1].stage = fragmentShader.GetShaderStage();
    m_ShaderStages[1].module = fragmentShader.GetShaderModule();
    m_ShaderStages[1].pName = "main";

    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetVertexInputDescription(const VertexInputDescription& description)
{
    m_BindingDescriptions = description.bindings;
    m_AttributeDescriptions = description.attributes;

    m_VertexInputState = {};
    m_VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    if (!m_BindingDescriptions.empty())
    {
        m_VertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(m_BindingDescriptions.size());
        m_VertexInputState.pVertexBindingDescriptions = m_BindingDescriptions.data();
    }

    if (!m_AttributeDescriptions.empty())
    {
        m_VertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_AttributeDescriptions.size());
        m_VertexInputState.pVertexAttributeDescriptions = m_AttributeDescriptions.data();
    }

    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetPrimitiveTopology(VkPrimitiveTopology topology)
{
    m_InputAssemblyState.topology = topology;
    return *this;
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
    m_MultisampleState.rasterizationSamples = samples;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetDepthTesting(bool enable, bool writeEnable, VkCompareOp compareOp)
{
    m_DepthStencilState.depthTestEnable = enable ? VK_TRUE : VK_FALSE;
    m_DepthStencilState.depthWriteEnable = writeEnable ? VK_TRUE : VK_FALSE;
    m_DepthStencilState.depthCompareOp = compareOp;
	m_DepthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetColorBlendAttachment(
        uint32_t attachmentIndex,
        bool blendEnable,
        VkColorComponentFlags colorWriteMask)
{
    if (attachmentIndex >= m_ColorBlendAttachmentStates.size())
    {
        throw std::runtime_error("Attachment index out of range");
    }

    auto& attachment = m_ColorBlendAttachmentStates[attachmentIndex];
    attachment.blendEnable = blendEnable ? VK_TRUE : VK_FALSE;
    attachment.colorWriteMask = colorWriteMask;

    if (blendEnable)
    {
        attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment.colorBlendOp = VK_BLEND_OP_ADD;
        attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetLayout(VkPipelineLayout layout)
{
    m_PipelineLayout = layout;
    return *this;
}

VulkanGraphicsPipelineBuilder& VulkanGraphicsPipelineBuilder::SetRenderPass(VulkanRenderPass* renderPass, uint32_t subpass)
{
    m_RenderPass = renderPass->GetHandle();
    m_Subpass = subpass;

    // Set up color blend attachments based on the render pass
    const auto& attachments = renderPass->GetAttachmentDescriptions();
    m_ColorBlendAttachmentStates.clear();
    for (const auto& attachment : attachments)
    {
        if (attachment.Type == AttachmentType::Color)
        {
            VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
            colorBlendAttachment.blendEnable = VK_FALSE;
            colorBlendAttachment.colorWriteMask = 0xf;
            m_ColorBlendAttachmentStates.push_back(colorBlendAttachment);
        }
    }

    m_ColorBlendState.attachmentCount = static_cast<uint32_t>(m_ColorBlendAttachmentStates.size());
    m_ColorBlendState.pAttachments = m_ColorBlendAttachmentStates.data();

    return *this;
}

std::shared_ptr<VulkanGraphicsPipeline> VulkanGraphicsPipelineBuilder::Build()
{
    auto pipeline = std::make_shared<VulkanGraphicsPipeline>(m_DebugName);
    pipeline->SetShaderStages(std::move(m_ShaderStages));
    pipeline->SetVertexInputState(m_VertexInputState, std::move(m_BindingDescriptions), std::move(m_AttributeDescriptions));
    pipeline->SetInputAssemblyState(m_InputAssemblyState);
    pipeline->SetViewportState(m_ViewportState);
    pipeline->SetRasterizationState(m_RasterizationState);
    pipeline->SetMultisampleState(m_MultisampleState);
    pipeline->SetDepthStencilState(m_DepthStencilState);
    pipeline->SetColorBlendState(m_ColorBlendState, std::move(m_ColorBlendAttachmentStates));
    pipeline->SetDynamicState(m_DynamicState, std::move(m_DynamicStates));
    pipeline->SetLayout(m_PipelineLayout);
    pipeline->SetRenderPass(m_RenderPass, m_Subpass);
    pipeline->Build();
    return pipeline;
}

void VulkanGraphicsPipeline::Bind(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
}
