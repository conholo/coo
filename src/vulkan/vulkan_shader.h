#pragma once

#include "vulkan_shader_reflection.h"

#include <string>
#include <vector>
#include <set>
#include <vulkan/vulkan.h>

enum class ShaderType
{
    Vertex,
    Fragment,
    Compute,
    Geometry,
    TessellationControl,
    TessellationEvaluation
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

class VulkanShader
{
public:
    VulkanShader(std::string filePath, ShaderType type);
    ~VulkanShader();

    VulkanShaderReflection& GetReflection() const { return *m_Reflection; }
    VkShaderModule GetShaderModule() const { return m_ShaderModule; }
    VkShaderStageFlagBits GetShaderStage() const;

private:
    void Load();
    std::vector<uint32_t> Compile();
    void CreateShaderModule(const std::vector<uint32_t>& code);

private:
    std::string m_FilePath;
    ShaderType m_Type;
    VkShaderModule m_ShaderModule = VK_NULL_HANDLE;
    std::string m_ShaderSource;

    std::shared_ptr<VulkanShaderReflection> m_Reflection;
};