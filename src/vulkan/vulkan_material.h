#pragma once

#include "vulkan_shader.h"
#include "vulkan_descriptors.h"

#include <vector>
#include <unordered_map>

class VulkanMaterial
{
public:
    VulkanMaterial(std::shared_ptr<VulkanShader> vertexShader, std::shared_ptr<VulkanShader> fragmentShader);


    void BindDescriptors();

    void UpdateDescriptorSet(uint32_t set, const std::vector<VkWriteDescriptorSet>& writes);
    void UpdateDescriptorSets(const std::vector<std::pair<uint32_t, std::vector<VkWriteDescriptorSet>>> &updates);

    VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
    const std::vector<VkDescriptorSet>& GetDescriptorSets() const { return m_DescriptorSets; }
    const std::vector<uint8_t>& GetPushConstantData() const { return m_PushConstantData; }

private:
    void CreateLayoutAndAllocateDescriptorSetsForShaderStage(VulkanShader& shaderRef,
                                                             std::vector<std::unique_ptr<VkDescriptorSet>>& outSets,
                                                             std::vector<std::unique_ptr<VulkanDescriptorSetLayout>>& outSetLayouts);
    void AllocateDescriptorSets();
    void CreatePipelineLayout();
    void SetPushConstants(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *data);


private:
    std::shared_ptr<VulkanShader> m_VertexShader;
    std::shared_ptr<VulkanShader> m_FragmentShader;

    VkPipelineLayout m_PipelineLayout;

    std::vector<uint8_t> m_PushConstantData;
    std::vector<VkPushConstantRange> m_PushConstantRanges;

    std::vector<VkDescriptorSet> m_DescriptorSets;
    std::vector<VulkanDescriptorSetLayout> m_DescriptorSetsLayouts;
    std::unique_ptr<VulkanDescriptorPool> m_DescriptorPool;
};