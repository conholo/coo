#pragma once

#include <vector>
#include <string>
#include <map>
#include <vulkan/vulkan.h>
#include <spirv_cross/spirv_cross.hpp>

class VulkanShaderReflection
{
public:
    struct ShaderResource
    {
        std::string name;
        uint32_t binding;
        uint32_t set;
        VkDescriptorType descriptorType;
        VkShaderStageFlags stageFlags;
        uint32_t arraySize;
        bool isReadOnly;
        bool isWriteOnly;

        std::string ToString() const;
    };

    struct VertexInputBinding
    {
        uint32_t binding;
        uint32_t stride;
        VkVertexInputRate inputRate;
    };

    struct VertexInputAttribute
    {
        uint32_t location;
        uint32_t binding;
        VkFormat format;
        uint32_t offset;
        std::string name;
    };

    struct ShaderOutput
    {
        std::string name;
        uint32_t location;
        VkFormat format;
    };

    struct PushConstantRange
    {
        std::string name;
        uint32_t offset;
        uint32_t size;
        VkShaderStageFlags stageFlags;
    };

    struct SpecializationConstant
    {
        uint32_t id;
        std::string name;
        uint32_t size;
    };

    VulkanShaderReflection(const std::vector<uint32_t>& spirvCode, VkShaderStageFlagBits stage);

    const std::string& GetEntryPoint() const { return m_EntryPoint; }
    const std::vector<ShaderResource>& GetResources() const { return m_Resources; }
    const std::map<uint32_t, std::vector<ShaderResource>>& GetDescriptorSets() const { return  m_DescriptorSets; }
    uint32_t GetDescriptorSetCount() const { return m_DescriptorSets.size(); }
    uint32_t GetTotalDescriptorCountAcrossAllSets() const;
    const std::vector<VertexInputBinding>& GetVertexInputBindings() const { return m_VertexInputBindings; }
    const std::vector<VertexInputAttribute>& GetVertexInputAttributes() const { return m_VertexInputAttributes; }
    const std::vector<ShaderOutput>& GetOutputs() const { return m_Outputs; }
    const std::vector<PushConstantRange>& GetPushConstantRanges() const { return m_PushConstantRanges; }
    const std::vector<SpecializationConstant>& GetSpecializationConstants() const { return m_SpecializationConstants; }

private:
    void ReflectEntryPoint(const spirv_cross::Compiler& compiler);
    void ReflectResources(const spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources);
    void ReflectVertexInputs(const spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources);
    void ReflectOutputs(const spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources);
    void ReflectPushConstants(const spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources);
    void ReflectSpecializationConstants(const spirv_cross::Compiler& compiler);

    static VkFormat SPIRTypeToVkFormat(const spirv_cross::SPIRType& type);
    static uint32_t GetFormatSize(VkFormat format);
    static std::string GetShaderStageName(VkShaderStageFlagBits stage);
    static std::string GetDescriptorTypeName(VkDescriptorType type);
    static std::string GetVkFormatName(VkFormat format);

    VkShaderStageFlagBits m_ShaderStage;
    std::string m_EntryPoint;
    std::vector<ShaderResource> m_Resources;
    std::vector<VertexInputBinding> m_VertexInputBindings;
    std::vector<VertexInputAttribute> m_VertexInputAttributes;
    std::vector<ShaderOutput> m_Outputs;
    std::vector<PushConstantRange> m_PushConstantRanges;
    std::vector<SpecializationConstant> m_SpecializationConstants;
    std::map<uint32_t, std::vector<ShaderResource>> m_DescriptorSets;
};