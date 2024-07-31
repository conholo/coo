#include "vulkan_shader_reflection.h"

VulkanShaderReflection::VulkanShaderReflection(const std::vector <uint32_t>& spirvCode, VkShaderStageFlagBits stage)
    : m_ShaderStage(stage)
{
    spirv_cross::Compiler compiler(spirvCode);
    ReflectResources(compiler);
    ReflectPushConstants(compiler);
    ReflectVertexInputs(compiler);
    ReflectDescriptorSets(compiler);
}

void VulkanShaderReflection::ReflectVertexInputs(const spirv_cross::Compiler &compiler)
{
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    for (const auto &resource: resources.stage_inputs)
    {
        uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
        const auto &type = compiler.get_type(resource.type_id);

        VertexInputAttribute attribute{};
        attribute.location = location;
        attribute.format = SPIRTypeToVkFormat(type);
        attribute.offset = 0;  // This will need to be calculated based on your vertex structure
        m_VertexInputAttributes.push_back(attribute);
    }

    // You may need to adjust this part based on your vertex input binding strategy
    if (!m_VertexInputAttributes.empty())
    {
        VertexInputBinding binding;
        binding.binding = 0;
        binding.stride = 0;  // This will need to be calculated based on your vertex structure
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        m_VertexInputBindings.push_back(binding);
    }
}

void VulkanShaderReflection::ReflectResources(const spirv_cross::Compiler &compiler)
{
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    auto reflectBinding =
    [&](const spirv_cross::Resource &resource, VkDescriptorType descriptorType)
    {
        ShaderResource shaderResource;
        shaderResource.name = resource.name;
        shaderResource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        shaderResource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        shaderResource.descriptorType = descriptorType;
        shaderResource.stageFlags = m_ShaderStage;

        const spirv_cross::SPIRType &type = compiler.get_type(resource.type_id);
        shaderResource.arraySize = type.array.empty() ? 1 : type.array[0];

        m_Resources.push_back(shaderResource);
    };

    for (const auto &resource: resources.uniform_buffers)
        reflectBinding(resource, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    for (const auto &resource: resources.storage_buffers)
        reflectBinding(resource, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    for (const auto &resource: resources.sampled_images)
        reflectBinding(resource, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    for (const auto &resource: resources.storage_images)
        reflectBinding(resource, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
}

void VulkanShaderReflection::ReflectPushConstants(const spirv_cross::Compiler &compiler)
{
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    for (const auto &resource: resources.push_constant_buffers)
    {
        const auto &bufferType = compiler.get_type(resource.base_type_id);

        for (uint32_t i = 0; i < bufferType.member_types.size(); ++i)
        {
            PushConstantRange range;
            range.name = compiler.get_member_name(bufferType.self, i);
            range.offset = compiler.get_member_decoration(bufferType.self, i, spv::DecorationOffset);
            range.size = compiler.get_declared_struct_member_size(bufferType, i);
            range.stageFlags = m_ShaderStage;

            m_PushConstantRanges.push_back(range);
        }
    }
}

VkDescriptorType VulkanShaderReflection::SPIRTypeToVkDescriptorType(const spirv_cross::SPIRType &type, bool isWritable)
{
    if (type.basetype == spirv_cross::SPIRType::SampledImage)
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    else if (type.basetype == spirv_cross::SPIRType::Image)
        return isWritable ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    else if (type.basetype == spirv_cross::SPIRType::Sampler)
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    else if (type.basetype == spirv_cross::SPIRType::Struct)
        return isWritable ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    // Add more types as needed...

    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

VkFormat VulkanShaderReflection::SPIRTypeToVkFormat(const spirv_cross::SPIRType &type)
{
    // Implement this based on the SPIR-V type
    // For example:
    if (type.basetype == spirv_cross::SPIRType::Float)
    {
        if (type.vecsize == 1) return VK_FORMAT_R32_SFLOAT;
        if (type.vecsize == 2) return VK_FORMAT_R32G32_SFLOAT;
        if (type.vecsize == 3) return VK_FORMAT_R32G32B32_SFLOAT;
        if (type.vecsize == 4) return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    // Add more type conversions as needed
    return VK_FORMAT_UNDEFINED;
}