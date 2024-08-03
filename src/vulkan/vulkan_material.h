#pragma once

#include "vulkan_shader.h"
#include "vulkan_descriptors.h"
#include "vulkan_image.h"
#include "vulkan_buffer.h"

#include <vector>
#include <unordered_map>
#include <set>

struct DescriptorUpdate
{
    uint32_t binding{};
    enum class Type { Buffer, Image } type{};
    union
    {
        VkDescriptorBufferInfo bufferInfo{};
        VkDescriptorImageInfo imageInfo;
    };
    // Optional fields for more specific updates
    VkDeviceSize offset = 0;
    VkDeviceSize range = VK_WHOLE_SIZE;
    uint8_t imageMip = 0;
};

struct PushConstantBlock
{
    std::string name;
    VkShaderStageFlags stageFlags;
    std::vector<uint8_t> data;
};

struct ShaderDescriptorInfo
{
    struct DescriptorInfo
    {
        VkDescriptorType type;
        uint32_t count;
        VkShaderStageFlags stageFlags;
        uint32_t binding;
    };

    std::map<uint32_t, std::vector<DescriptorInfo>> setDescriptors;
    std::map<VkDescriptorType, uint32_t> totalDescriptorCounts;
    std::set<uint32_t> uniqueSets;

    void AddShaderReflection(const VulkanShaderReflection& reflection, VkShaderStageFlags stage)
    {
        for (const auto& [set, resources] : reflection.GetDescriptorSets())
        {
            uniqueSets.insert(set);
            for (const auto& resource : resources)
            {
                auto& descriptors = setDescriptors[set];
                auto it = std::find_if(descriptors.begin(), descriptors.end(),
                                       [&](const DescriptorInfo& info)
                                       {
                                           return info.type == resource.descriptorType &&
                                                  info.binding == resource.binding;
                                       });
                if (it != descriptors.end())
                {
                    it->stageFlags |= stage;
                    it->count = std::max(it->count, resource.arraySize);
                }
                else
                {
                    descriptors.push_back({.type = resource.descriptorType,
                                           .count = resource.arraySize,
                                           .stageFlags = stage,
                                           .binding = resource.binding});
                }
                totalDescriptorCounts[resource.descriptorType] += resource.arraySize;
            }
        }
    }

    uint32_t GetTotalUniqueSetCount() const
    {
        return uniqueSets.size();
    }
};

class VulkanMaterial
{
public:
    VulkanMaterial(std::shared_ptr<VulkanShader> vertexShader, std::shared_ptr<VulkanShader> fragmentShader);

    void BindDescriptors(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint);
    void UpdateDescriptor(uint32_t set, const DescriptorUpdate& update);
    void UpdateDescriptorSets(const std::vector<std::pair<uint32_t, std::vector<DescriptorUpdate>>>& updates);

    template<typename T>
    void SetPushConstant(const std::string& name, const T& value);
    void BindPushConstants(VkCommandBuffer commandBuffer);


    VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

    std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> GetDescriptorSetLayouts() { return m_DescriptorSetsLayouts; }
    std::vector<std::shared_ptr<VkDescriptorSet>> DescriptorSets() { return m_DescriptorSets; }

private:
    void CreateLayoutsAndAllocateDescriptorSets();
    void AllocateDescriptorSets();
    void CreatePipelineLayout();
    void InitializePushConstants();

private:
    std::shared_ptr<VulkanShader> m_VertexShader;
    std::shared_ptr<VulkanShader> m_FragmentShader;

    VkPipelineLayout m_PipelineLayout;

    std::vector<PushConstantBlock> m_PushConstantBlocks;
    std::unordered_map<std::string, size_t> m_PushConstantBlockIndex;

    ShaderDescriptorInfo m_ShaderDescriptorInfo;
    std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_DescriptorSetsLayouts;
    std::vector<std::shared_ptr<VkDescriptorSet>> m_DescriptorSets;

    std::unique_ptr<VulkanDescriptorPool> m_DescriptorPool;
};