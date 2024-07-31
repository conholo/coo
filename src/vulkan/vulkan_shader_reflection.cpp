#include "vulkan_shader_reflection.h"

VulkanShaderReflection::VulkanShaderReflection(const std::vector <uint32_t>& spirvCode, VkShaderStageFlagBits stage)
    : m_ShaderStage(stage)
{
    spirv_cross::Compiler compiler(spirvCode);
    ReflectResources(compiler);
    ReflectPushConstants(compiler);
    ReflectVertexInputs(compiler);
}

void VulkanShaderReflection::ReflectResources(const spirv_cross::Compiler& compiler)
{
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    auto reflectBinding =
            [&](const spirv_cross::Resource& resource, VkDescriptorType descriptorType)
    {
        ShaderResource shaderResource;
        shaderResource.name = resource.name;
        shaderResource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        shaderResource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        shaderResource.descriptorType = descriptorType;
        shaderResource.stageFlags = m_ShaderStage;

        const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
        shaderResource.arraySize = type.array.empty() ? 1 : type.array[0];

        m_Resources.push_back(shaderResource);

        // Create and add the corresponding VkDescriptorSetLayoutBinding
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = shaderResource.binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = shaderResource.arraySize;
        layoutBinding.stageFlags = m_ShaderStage;

        m_DescriptorSetLayoutBindings.push_back(layoutBinding);
    };

    for (const auto& resource : resources.uniform_buffers)
        reflectBinding(resource, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    for (const auto& resource : resources.storage_buffers)
        reflectBinding(resource, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    for (const auto& resource : resources.sampled_images)
        reflectBinding(resource, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    for (const auto& resource : resources.storage_images)
        reflectBinding(resource, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    for (const auto& resource : resources.separate_samplers)
        reflectBinding(resource, VK_DESCRIPTOR_TYPE_SAMPLER);

    for (const auto& resource : resources.separate_images)
        reflectBinding(resource, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
}

void VulkanShaderReflection::ReflectVertexInputs(const spirv_cross::Compiler &compiler)
{
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    VertexInputBinding binding{};
    binding.binding = 0;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    binding.stride = 0;  // We'll calculate this

    for (const auto& resource : resources.stage_inputs)
    {
        uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
        const auto& type = compiler.get_type(resource.type_id);

        VertexInputAttribute attribute;
        attribute.location = location;
        attribute.format = SPIRTypeToVkFormat(type);
        attribute.offset = binding.stride;  // Current stride is the offset for this attribute
        attribute.name = resource.name;

        m_VertexInputAttributes.push_back(attribute);

        // Update the stride
        binding.stride += GetFormatSize(attribute.format);
    }

    m_VertexInputBindings.push_back(binding);
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

VkFormat VulkanShaderReflection::SPIRTypeToVkFormat(const spirv_cross::SPIRType &type)
{
    switch (type.basetype)
    {
        case spirv_cross::SPIRType::Float:
            switch (type.vecsize)
            {
                case 1: return VK_FORMAT_R32_SFLOAT;
                case 2: return VK_FORMAT_R32G32_SFLOAT;
                case 3: return VK_FORMAT_R32G32B32_SFLOAT;
                case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            }
            break;
        case spirv_cross::SPIRType::Int:
            switch (type.vecsize)
            {
                case 1: return VK_FORMAT_R32_SINT;
                case 2: return VK_FORMAT_R32G32_SINT;
                case 3: return VK_FORMAT_R32G32B32_SINT;
                case 4: return VK_FORMAT_R32G32B32A32_SINT;
            }
            break;
        case spirv_cross::SPIRType::UInt:
            switch (type.vecsize)
            {
                case 1: return VK_FORMAT_R32_UINT;
                case 2: return VK_FORMAT_R32G32_UINT;
                case 3: return VK_FORMAT_R32G32B32_UINT;
                case 4: return VK_FORMAT_R32G32B32A32_UINT;
            }
            break;
        case spirv_cross::SPIRType::Half:
            switch (type.vecsize)
            {
                case 1: return VK_FORMAT_R16_SFLOAT;
                case 2: return VK_FORMAT_R16G16_SFLOAT;
                case 3: return VK_FORMAT_R16G16B16_SFLOAT;
                case 4: return VK_FORMAT_R16G16B16A16_SFLOAT;
            }
            break;
        case spirv_cross::SPIRType::Short:
            switch (type.vecsize)
            {
                case 1: return VK_FORMAT_R16_SINT;
                case 2: return VK_FORMAT_R16G16_SINT;
                case 3: return VK_FORMAT_R16G16B16_SINT;
                case 4: return VK_FORMAT_R16G16B16A16_SINT;
            }
            break;
        case spirv_cross::SPIRType::UShort:
            switch (type.vecsize)
            {
                case 1: return VK_FORMAT_R16_UINT;
                case 2: return VK_FORMAT_R16G16_UINT;
                case 3: return VK_FORMAT_R16G16B16_UINT;
                case 4: return VK_FORMAT_R16G16B16A16_UINT;
            }
            break;
        case spirv_cross::SPIRType::Double:
            switch (type.vecsize)
            {
                case 1: return VK_FORMAT_R64_SFLOAT;
                case 2: return VK_FORMAT_R64G64_SFLOAT;
                case 3: return VK_FORMAT_R64G64B64_SFLOAT;
                case 4: return VK_FORMAT_R64G64B64A64_SFLOAT;
            }
            break;
        case spirv_cross::SPIRType::Unknown:
        case spirv_cross::SPIRType::Void:
            return VK_FORMAT_UNDEFINED;
        case spirv_cross::SPIRType::Boolean:
            return VK_FORMAT_R8_UINT;  // Booleans are typically 1 byte
        case spirv_cross::SPIRType::SByte:
            return VK_FORMAT_R8_SINT;
        case spirv_cross::SPIRType::UByte:
            return VK_FORMAT_R8_UINT;
        case spirv_cross::SPIRType::Int64:
            switch (type.vecsize)
            {
                case 1: return VK_FORMAT_R64_SINT;
                case 2: return VK_FORMAT_R64G64_SINT;
                case 3: return VK_FORMAT_R64G64B64_SINT;
                case 4: return VK_FORMAT_R64G64B64A64_SINT;
            }
            break;
        case spirv_cross::SPIRType::UInt64:
            switch (type.vecsize)
            {
                case 1: return VK_FORMAT_R64_UINT;
                case 2: return VK_FORMAT_R64G64_UINT;
                case 3: return VK_FORMAT_R64G64B64_UINT;
                case 4: return VK_FORMAT_R64G64B64A64_UINT;
            }
            break;
        case spirv_cross::SPIRType::AtomicCounter:
            return VK_FORMAT_R32_UINT;  // Atomic counters are typically 32-bit

        case spirv_cross::SPIRType::Struct:
            return VK_FORMAT_UNDEFINED;  // Structs don't have a specific format

        case spirv_cross::SPIRType::Image:
        case spirv_cross::SPIRType::SampledImage:
        case spirv_cross::SPIRType::Sampler:
            return VK_FORMAT_UNDEFINED;  // These types don't have a specific format

        case spirv_cross::SPIRType::AccelerationStructure:
        case spirv_cross::SPIRType::RayQuery:
            return VK_FORMAT_UNDEFINED;  // Ray tracing types don't have a specific format

        case spirv_cross::SPIRType::ControlPointArray:
        case spirv_cross::SPIRType::Interpolant:
            return VK_FORMAT_UNDEFINED;  // These types don't have a direct Vulkan format

        case spirv_cross::SPIRType::Char:
            return VK_FORMAT_R8_SINT;
        default:
            return VK_FORMAT_UNDEFINED;
    }
    return VK_FORMAT_UNDEFINED;
}

uint32_t VulkanShaderReflection::GetFormatSize(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_UINT:
            return 4;
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_UINT:
            return 8;
        case VK_FORMAT_R32G32B32_SFLOAT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_UINT:
            return 12;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_UINT:
            return 16;
        case VK_FORMAT_R16_SFLOAT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_UINT:
            return 2;
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_UINT:
            return 4;
        case VK_FORMAT_R16G16B16_SFLOAT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_UINT:
            return 6;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_UINT:
            return 8;
        case VK_FORMAT_R64_SFLOAT:
            return 8;
        case VK_FORMAT_R64G64_SFLOAT:
            return 16;
        case VK_FORMAT_R64G64B64_SFLOAT:
            return 24;
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            return 32;
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
            return 1;
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64_UINT:
            return 8;
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64_UINT:
            return 16;
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_UINT:
            return 24;
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_UINT:
            return 32;
        default:
            return 0; // Unknown format
    }
}

