#include "vulkan_graphics_pipeline.h"
#include "vulkan_context.h"
#include "vulkan_utils.h"

VulkanGraphicsPipeline::VulkanGraphicsPipeline(std::string debugName)
    : m_DebugName(std::move(debugName))
{}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    if (m_Pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(VulkanContext::Get().Device(), m_Pipeline, nullptr);
    }
}

void VulkanGraphicsPipeline::SetShaderStages(const std::vector<VkPipelineShaderStageCreateInfo> &shaderStages)
{
    m_ShaderStages = shaderStages;
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

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
            VulkanContext::Get().Device(),
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &m_Pipeline));
}