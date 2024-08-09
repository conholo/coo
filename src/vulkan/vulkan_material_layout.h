#pragma once

#include "vulkan_shader.h"
#include "vulkan_descriptors.h"
#include <vector>
#include <memory>

class VulkanMaterialLayout
{
public:
    struct PushConstantRange
    {
        std::string name;
        VkShaderStageFlags stageFlags;
        uint32_t offset;
        uint32_t size;
    };

    VulkanMaterialLayout(std::shared_ptr<VulkanShader> vertexShader, std::shared_ptr<VulkanShader> fragmentShader);
    ~VulkanMaterialLayout();

    VulkanMaterialLayout(const VulkanMaterialLayout&) = delete;
    VulkanMaterialLayout& operator=(const VulkanMaterialLayout&) = delete;

    const ShaderDescriptorInfo& GetShaderDescriptorInfo() const { return m_ShaderDescriptorInfo; }
    const std::vector<std::shared_ptr<VulkanDescriptorSetLayout>>& GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }
    const std::vector<PushConstantRange>& GetPushConstantRanges() const { return m_PushConstantRanges; }

    VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

private:
    void CreateDescriptorSetLayouts();
    void CreatePipelineLayout();
    void ProcessPushConstants();

    std::shared_ptr<VulkanShader> m_VertexShader;
    std::shared_ptr<VulkanShader> m_FragmentShader;
    ShaderDescriptorInfo m_ShaderDescriptorInfo;
    std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_DescriptorSetLayouts;
    std::vector<PushConstantRange> m_PushConstantRanges;
    VkPipelineLayout m_PipelineLayout;
};
