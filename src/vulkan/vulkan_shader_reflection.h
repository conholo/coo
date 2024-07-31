#include <spirv_cross/spirv_cross.hpp>
#include <vector>
#include <string>
#include <vulkan/vulkan.h>

struct ShaderResource
{
    std::string name;
    uint32_t binding;
    uint32_t set;
    VkDescriptorType descriptorType;
    VkShaderStageFlags stageFlags;
    uint32_t arraySize;
};

struct PushConstantRange
{
    std::string name;
    uint32_t offset;
    uint32_t size;
    VkShaderStageFlags stageFlags;
};

struct VertexInputAttribute
{
    uint32_t location;
    VkFormat format;
    uint32_t offset;
};

struct VertexInputBinding
{
    uint32_t binding;
    uint32_t stride;
    VkVertexInputRate inputRate;
};

class VulkanShaderReflection
{
public:
    VulkanShaderReflection(const std::vector<uint32_t> &spirvCode, VkShaderStageFlagBits stage);

    const std::vector<ShaderResource>& GetResources() const { return m_Resources; }
    const std::vector<PushConstantRange> &GetPushConstantRanges() const { return m_PushConstantRanges; }
    VkShaderStageFlags GetShaderStage() const { return m_ShaderStage; }


    static VkDescriptorType SPIRTypeToVkDescriptorType(const spirv_cross::SPIRType &type, bool isWritable);
    static VkFormat SPIRTypeToVkFormat(const spirv_cross::SPIRType &type);

private:
    void ReflectVertexInputs(const spirv_cross::Compiler &compiler);
    void ReflectDescriptorSets(const spirv_cross::Compiler &compiler);
    void ReflectResources(const spirv_cross::Compiler &compiler);
    void ReflectPushConstants(const spirv_cross::Compiler &compiler);


    std::vector<ShaderResource> m_Resources;
    std::vector<PushConstantRange> m_PushConstantRanges;
    std::vector<VertexInputAttribute> m_VertexInputAttributes;
    std::vector<VertexInputBinding> m_VertexInputBindings;
    std::vector<VkDescriptorSetLayoutBinding> m_DescriptorSetLayoutBindings;

    VkShaderStageFlags m_ShaderStage;
};