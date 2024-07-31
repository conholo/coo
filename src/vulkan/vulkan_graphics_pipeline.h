#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class VulkanGraphicsPipeline
{
public:
    VulkanGraphicsPipeline(std::string debugName = "GraphicsPipeline");

    ~VulkanGraphicsPipeline();

    void SetShaderStages(const std::vector <VkPipelineShaderStageCreateInfo> &shaderStages);
    void SetVertexInputState(const VkPipelineVertexInputStateCreateInfo &vertexInputState);
    void SetInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo &inputAssemblyState);
    void SetViewportState(const VkPipelineViewportStateCreateInfo &viewportState);
    void SetRasterizationState(const VkPipelineRasterizationStateCreateInfo &rasterizationState);
    void SetMultisampleState(const VkPipelineMultisampleStateCreateInfo &multisampleState);
    void SetDepthStencilState(const VkPipelineDepthStencilStateCreateInfo &depthStencilState);
    void SetColorBlendState(const VkPipelineColorBlendStateCreateInfo &colorBlendState);
    void SetDynamicState(const VkPipelineDynamicStateCreateInfo &dynamicState);
    void SetLayout(VkPipelineLayout layout);
    void SetRenderPass(VkRenderPass renderPass, uint32_t subpass = 0);

    void Build();

    VkPipeline GraphicsPipeline() const { return m_Pipeline; }

private:
    std::string m_DebugName;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    uint32_t m_Subpass = 0;

    std::vector <VkPipelineShaderStageCreateInfo> m_ShaderStages;
    VkPipelineVertexInputStateCreateInfo m_VertexInputState{};
    VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyState{};
    VkPipelineViewportStateCreateInfo m_ViewportState{};
    VkPipelineRasterizationStateCreateInfo m_RasterizationState{};
    VkPipelineMultisampleStateCreateInfo m_MultisampleState{};
    VkPipelineDepthStencilStateCreateInfo m_DepthStencilState{};
    VkPipelineColorBlendStateCreateInfo m_ColorBlendState{};
    VkPipelineDynamicStateCreateInfo m_DynamicState{};
};