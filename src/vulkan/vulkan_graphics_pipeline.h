// vulkan_graphics_pipeline.h

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>
#include "vulkan_shader.h"
#include "vulkan_render_pass.h"

struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

class VulkanGraphicsPipeline {
public:
    explicit VulkanGraphicsPipeline(std::string debugName = "GraphicsPipeline");
    ~VulkanGraphicsPipeline();

    VulkanGraphicsPipeline(const VulkanGraphicsPipeline&) = delete;
    VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline&) = delete;
    VulkanGraphicsPipeline(VulkanGraphicsPipeline&&) noexcept;
    VulkanGraphicsPipeline& operator=(VulkanGraphicsPipeline&&) noexcept;

    void SetShaderStages(std::vector<VkPipelineShaderStageCreateInfo>&& shaderStages);
    void SetVertexInputState(const VkPipelineVertexInputStateCreateInfo& vertexInputState,
                             std::vector<VkVertexInputBindingDescription>&& bindings,
                             std::vector<VkVertexInputAttributeDescription>&& attributes);
    void SetInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo& inputAssemblyState);
    void SetViewportState(const VkPipelineViewportStateCreateInfo& viewportState);
    void SetRasterizationState(const VkPipelineRasterizationStateCreateInfo& rasterizationState);
    void SetMultisampleState(const VkPipelineMultisampleStateCreateInfo& multisampleState);
    void SetDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& depthStencilState);
    void SetColorBlendState(const VkPipelineColorBlendStateCreateInfo& colorBlendState,
                            std::vector<VkPipelineColorBlendAttachmentState>&& colorBlendAttachments);
    void SetDynamicState(const VkPipelineDynamicStateCreateInfo& dynamicState,
                         std::vector<VkDynamicState>&& dynamicStates);
    void SetLayout(VkPipelineLayout layout);
    void SetRenderPass(VkRenderPass renderPass, uint32_t subpass = 0);

    void Build();
    VkPipeline GetPipeline() const { return m_Pipeline; }

private:
    std::string m_DebugName;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    uint32_t m_Subpass = 0;

    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
    VkPipelineVertexInputStateCreateInfo m_VertexInputState{};
    std::vector<VkVertexInputBindingDescription> m_BindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> m_AttributeDescriptions;
    VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyState{};
    VkPipelineViewportStateCreateInfo m_ViewportState{};
    VkPipelineRasterizationStateCreateInfo m_RasterizationState{};
    VkPipelineMultisampleStateCreateInfo m_MultisampleState{};
    VkPipelineDepthStencilStateCreateInfo m_DepthStencilState{};
    VkPipelineColorBlendStateCreateInfo m_ColorBlendState{};
    std::vector<VkPipelineColorBlendAttachmentState> m_ColorBlendAttachments;
    VkPipelineDynamicStateCreateInfo m_DynamicState{};
    std::vector<VkDynamicState> m_DynamicStates;
};

class VulkanGraphicsPipelineBuilder {
public:
    explicit VulkanGraphicsPipelineBuilder(std::string debugName = "GraphicsPipeline");

    VulkanGraphicsPipelineBuilder& SetShaders(const std::shared_ptr<VulkanShader>& vertexShader, const std::shared_ptr<VulkanShader>& fragmentShader);
    VulkanGraphicsPipelineBuilder& SetVertexInputDescription(const VertexInputDescription& description);
    VulkanGraphicsPipelineBuilder& SetPrimitiveTopology(VkPrimitiveTopology topology);
    VulkanGraphicsPipelineBuilder& SetPolygonMode(VkPolygonMode mode);
    VulkanGraphicsPipelineBuilder& SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    VulkanGraphicsPipelineBuilder& SetMultisampling(VkSampleCountFlagBits samples);
    VulkanGraphicsPipelineBuilder& SetDepthTesting(bool enable, bool writeEnable, VkCompareOp compareOp);
    VulkanGraphicsPipelineBuilder& SetColorBlendAttachment(uint32_t attachmentIndex, bool blendEnable, VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
    VulkanGraphicsPipelineBuilder& SetDynamicStates(const std::vector<VkDynamicState>& dynamicStates);
    VulkanGraphicsPipelineBuilder& SetLayout(VkPipelineLayout layout);
    VulkanGraphicsPipelineBuilder& SetRenderPass(const VulkanRenderPass* renderPass, uint32_t subpass = 0);

    std::unique_ptr<VulkanGraphicsPipeline> Build();

private:
    std::string m_DebugName;
    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
    VkPipelineVertexInputStateCreateInfo m_VertexInputState{};
    std::vector<VkVertexInputBindingDescription> m_BindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> m_AttributeDescriptions;
    VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyState{};
    VkPipelineViewportStateCreateInfo m_ViewportState{};
    VkPipelineRasterizationStateCreateInfo m_RasterizationState{};
    VkPipelineMultisampleStateCreateInfo m_MultisampleState{};
    VkPipelineDepthStencilStateCreateInfo m_DepthStencilState{};
    VkPipelineColorBlendStateCreateInfo m_ColorBlendState{};
    std::vector<VkPipelineColorBlendAttachmentState> m_ColorBlendAttachmentStates;
    VkPipelineDynamicStateCreateInfo m_DynamicState{};
    std::vector<VkDynamicState> m_DynamicStates;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    uint32_t m_Subpass = 0;

    void SetupDefaultStates();
};