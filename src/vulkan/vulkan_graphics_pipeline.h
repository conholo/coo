#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include "vulkan_shader.h"


class VulkanGraphicsPipeline
{
public:
    explicit VulkanGraphicsPipeline(std::string debugName = "GraphicsPipeline");

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

struct VertexInputDescription
{
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

class VulkanGraphicsPipelineBuilder
{
public:
    VulkanGraphicsPipelineBuilder(std::string debugName = "GraphicsPipeline");
    VulkanGraphicsPipelineBuilder& SetShaders(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);

    VulkanGraphicsPipelineBuilder& SetVertexInputDescription(const VertexInputDescription &description);
    VulkanGraphicsPipelineBuilder& SetPrimitiveTopology(VkPrimitiveTopology topology);
    VulkanGraphicsPipelineBuilder& SetPolygonMode(VkPolygonMode mode);
    VulkanGraphicsPipelineBuilder& SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    VulkanGraphicsPipelineBuilder& SetMultisampling(VkSampleCountFlagBits samples);
    VulkanGraphicsPipelineBuilder& SetDepthTesting(bool enable, bool writeEnable, VkCompareOp compareOp);
    VulkanGraphicsPipelineBuilder& SetColorBlendAttachment(bool blendEnable, VkColorComponentFlags colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT);

    VulkanGraphicsPipelineBuilder& SetDynamicStates(const std::vector<VkDynamicState> &dynamicStates);
    VulkanGraphicsPipelineBuilder& SetLayout(VkPipelineLayout layout);
    VulkanGraphicsPipelineBuilder& SetRenderPass(VkRenderPass renderPass, uint32_t subpass = 0);
    std::unique_ptr<VulkanGraphicsPipeline> Build();

private:
    VulkanGraphicsPipeline m_Pipeline;
    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
    VkPipelineVertexInputStateCreateInfo m_VertexInputState{};
    VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyState{};
    VkViewport m_Viewport{};
    VkRect2D m_Scissor{};
    VkPipelineViewportStateCreateInfo m_ViewportState{};
    VkPipelineRasterizationStateCreateInfo m_RasterizationState{};
    VkPipelineMultisampleStateCreateInfo m_MultisampleState{};
    VkPipelineDepthStencilStateCreateInfo m_DepthStencilState{};
    VkPipelineColorBlendStateCreateInfo m_ColorBlendState{};
    VkPipelineColorBlendAttachmentState m_ColorBlendAttachment{};
    VkPipelineDynamicStateCreateInfo m_DynamicState{};
    std::vector<VkDynamicState> m_DynamicStates;

    std::unique_ptr<VulkanShader> m_VertexShader;
    std::unique_ptr<VulkanShader> m_FragmentShader;

    std::string m_DebugName{};
    void SetupDefaultStates();
};
