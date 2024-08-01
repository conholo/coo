#pragma once

#include "vulkan_shader.h"
#include "vulkan_descriptors.h"

class VulkanMaterial
{
public:
    VulkanMaterial(std::shared_ptr<VulkanShader> vertexShader, std::shared_ptr<VulkanShader> fragmentShader);

    void CreateDescriptorSetLayout();
    void AllocateDescriptorSets();
    void UpdateDescriptorSet(uint32_t set, const std::vector<VkWriteDescriptorSet>& writes);
    void UpdateDescriptorSets(const std::vector<std::pair<uint32_t, std::vector<VkWriteDescriptorSet>>> &updates);
    void CreatePipelineLayout();
    void SetPushConstants(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *data);

    VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
    const std::vector<VkDescriptorSet>& GetDescriptorSets() const { return m_DescriptorSets; }
    const std::vector<uint8_t>& GetPushConstantData() const { return m_PushConstantData; }

private:
    std::shared_ptr<VulkanShader> m_VertexShader;
    std::shared_ptr<VulkanShader> m_FragmentShader;

    VkPipelineLayout m_PipelineLayout;

    std::vector<uint8_t> m_PushConstantData;
    std::vector<VkPushConstantRange> m_PushConstantRanges;

    std::vector<VkDescriptorSet> m_DescriptorSets;
    std::unique_ptr<VulkanDescriptorSetLayout> m_DescriptorSetLayout;
    std::unique_ptr<VulkanDescriptorPool> m_DescriptorPool;
};