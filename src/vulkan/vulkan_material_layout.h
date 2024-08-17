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

    VulkanMaterialLayout(VulkanShader& vertShader, VulkanShader& fragShader, std::string debugName);
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
    void ProcessPushConstants(VulkanShader& vertShader, VulkanShader& fragShader);

	std::string m_DebugName;
    ShaderDescriptorInfo m_ShaderDescriptorInfo;
    std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_DescriptorSetLayouts;
    std::vector<PushConstantRange> m_PushConstantRanges;
    VkPipelineLayout m_PipelineLayout{};
};
