#include <iostream>
#include <sstream>
#include <numeric>
#include "vulkan_shader_reflection.h"

VulkanShaderReflection::VulkanShaderReflection(const std::vector<uint32_t>& spirvCode, VkShaderStageFlagBits stage)
        : m_ShaderStage(stage)
{
    spirv_cross::Compiler compiler(spirvCode);
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    std::cout << "Reflecting shader of stage: " << GetShaderStageName(stage) << std::endl;

    ReflectEntryPoint(compiler);
	ReflectDescriptors(compiler, resources);
    ReflectPushConstants(compiler, resources);
    ReflectVertexInputs(compiler, resources);
    ReflectOutputs(compiler, resources);
    ReflectSpecializationConstants(compiler);
}

void VulkanShaderReflection::ReflectEntryPoint(const spirv_cross::Compiler& compiler)
{
    m_EntryPoint = compiler.get_entry_points_and_stages()[0].name;
    std::cout << "Entry point: " << m_EntryPoint << std::endl;
}

void VulkanShaderReflection::ReflectDescriptors(const spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources)
{
    auto reflectResource = [&](const spirv_cross::Resource& resource, VkDescriptorType descriptorType)
    {
        ShaderResource shaderResource;
        shaderResource.name = resource.name;
        shaderResource.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        shaderResource.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        shaderResource.descriptorType = descriptorType;
        shaderResource.stageFlags = m_ShaderStage;

        const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
        shaderResource.arraySize = type.array.empty() ? 1 : type.array[0];

        shaderResource.isReadOnly = compiler.has_decoration(resource.id, spv::DecorationNonWritable);
        shaderResource.isWriteOnly = compiler.has_decoration(resource.id, spv::DecorationNonReadable);

        m_Resources.push_back(shaderResource);
        m_DescriptorSets[shaderResource.set].push_back(shaderResource);

        std::cout << shaderResource.ToString() << "\n";
    };

    auto isDescriptor = [&](
            const spirv_cross::Resource& resource,
            const spirv_cross::Compiler& compiler,
            VkDescriptorType descriptorType)
    {
        bool hasBinding = compiler.has_decoration(resource.id, spv::DecorationBinding);
        if(hasBinding)
        {
            bool hasSetIdentifier = compiler.has_decoration(resource.id, spv::DecorationDescriptorSet);
            if(!hasSetIdentifier)
            {
                std::string type = GetDescriptorTypeName(descriptorType);
                uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                std::cout << "Warning: Resource " << resource.name << " with binding: " << binding << " and type " << type << " does not explicitly specify a descriptor set.  The engine requires that the set corresponding to the same identifier of the host code be present in the shader." << "\n";
                return false;
            }
            return true;
        }
        return false;
    };

    for (const auto& resource : resources.uniform_buffers)
    {
        if(!isDescriptor(resource, compiler, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)) continue;
        reflectResource(resource, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }

    for (const auto& resource : resources.storage_buffers)
    {
        if(!isDescriptor(resource, compiler, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)) continue;
        reflectResource(resource, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    }

    for (const auto& resource : resources.sampled_images)
    {
        if(!isDescriptor(resource, compiler, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)) continue;
        reflectResource(resource, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    for (const auto& resource : resources.storage_images)
    {
        if(!isDescriptor(resource, compiler, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)) continue;
        reflectResource(resource, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    }

    for (const auto& resource : resources.separate_samplers)
    {
        if(!isDescriptor(resource, compiler, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)) continue;
        reflectResource(resource, VK_DESCRIPTOR_TYPE_SAMPLER);
    }

    for (const auto& resource : resources.separate_images)
    {
        if(!isDescriptor(resource, compiler, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)) continue;
        reflectResource(resource, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    }
}

void VulkanShaderReflection::ReflectVertexInputs(const spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources)
{
    if (m_ShaderStage != VK_SHADER_STAGE_VERTEX_BIT)
    {
        return;
    }

    VertexInputBinding binding{};
    binding.binding = 0;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    binding.stride = 0;

    for (const auto& resource : resources.stage_inputs)
    {
        uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
        const auto& type = compiler.get_type(resource.type_id);

        VertexInputAttribute attribute;
        attribute.location = location;
        attribute.format = SPIRTypeToVkFormat(type);
        attribute.offset = binding.stride;

        // Try to get the name in different ways
        attribute.name = resource.name;
        if (attribute.name.empty())
        {
            // Try to get the HLSL semantic, if available
            if (compiler.has_decoration(resource.id, spv::DecorationHlslSemanticGOOGLE))
            {
                attribute.name = compiler.get_decoration_string(resource.id, spv::DecorationHlslSemanticGOOGLE);
            }
            // If still empty, use the built-in type if it's a built-in variable
            else if (compiler.has_decoration(resource.id, spv::DecorationBuiltIn))
            {
                auto builtIn = static_cast<spv::BuiltIn>(compiler.get_decoration(resource.id, spv::DecorationBuiltIn));
                attribute.name = "builtin_" + std::to_string(builtIn);
            }
            // If still empty, create a name based on the location
            else
            {
                attribute.name = "input_location_" + std::to_string(location);
            }
        }
        m_VertexInputAttributes.push_back(attribute);

        binding.stride += GetFormatSize(attribute.format);

        std::cout << "Vertex Input: " << attribute.name
                  << " (Location: " << attribute.location
                  << ", Format: " << GetVkFormatName(attribute.format)
                  << ", Offset: " << attribute.offset << ")" << std::endl;
    }

    if (!m_VertexInputAttributes.empty())
    {
        m_VertexInputBindings.push_back(binding);
        std::cout << "Vertex Input Binding: Stride = " << binding.stride << std::endl;
    }
}

void VulkanShaderReflection::ReflectOutputs(const spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources)
{
    for (const auto& resource : resources.stage_outputs)
    {
        uint32_t location = compiler.get_decoration(resource.id, spv::DecorationLocation);
        const auto& type = compiler.get_type(resource.type_id);

        ShaderOutput output;
        output.name = resource.name;
        output.location = location;
        output.format = SPIRTypeToVkFormat(type);

        m_Outputs.push_back(output);

        std::cout << "Shader Output: " << output.name
                  << " (Location: " << output.location
                  << ", Format: " << GetVkFormatName(output.format) << ")" << std::endl;
    }
}

void VulkanShaderReflection::ReflectPushConstants(const spirv_cross::Compiler& compiler, const spirv_cross::ShaderResources& resources)
{
    for (const auto& resource : resources.push_constant_buffers)
    {
        const auto& bufferType = compiler.get_type(resource.base_type_id);

        for (uint32_t i = 0; i < bufferType.member_types.size(); ++i)
        {
            PushConstantRange range;
			range.pushStructName = resource.name;
            range.name = compiler.get_member_name(bufferType.self, i);
            range.offset = compiler.get_member_decoration(bufferType.self, i, spv::DecorationOffset);
            range.size = compiler.get_declared_struct_member_size(bufferType, i);
            range.stageFlags = m_ShaderStage;

            m_PushConstantRanges.push_back(range);

			std::cout << "Push Constant Range: " << range.name
					  << ", Stage: " << range.stageFlags
					  << ", Offset: " << range.offset
					  << ", Size: " << range.size << std::endl;        }
    }
}

void VulkanShaderReflection::ReflectSpecializationConstants(const spirv_cross::Compiler& compiler)
{
    auto specConstants = compiler.get_specialization_constants();
    for (const auto& constant : specConstants)
    {
        SpecializationConstant specConstant;
        specConstant.id = constant.constant_id;
        specConstant.name = compiler.get_name(constant.id);

        // Get the SPIRConstant for this specialization constant
        auto& spirConstant = compiler.get_constant(constant.id);

        // Determine the size based on the constant type
        switch (spirConstant.constant_type)
        {
            case spirv_cross::SPIRType::Boolean:
            case spirv_cross::SPIRType::Char:
            case spirv_cross::SPIRType::SByte:
            case spirv_cross::SPIRType::UByte:
                specConstant.size = 1;
                break;
            case spirv_cross::SPIRType::Short:
            case spirv_cross::SPIRType::UShort:
                specConstant.size = 2;
                break;
            case spirv_cross::SPIRType::Int:
            case spirv_cross::SPIRType::UInt:
            case spirv_cross::SPIRType::Float:
                specConstant.size = 4;
                break;
            case spirv_cross::SPIRType::Int64:
            case spirv_cross::SPIRType::UInt64:
            case spirv_cross::SPIRType::Double:
                specConstant.size = 8;
                break;
            default:
                specConstant.size = 0;  // Unknown type
                break;
        }

        m_SpecializationConstants.push_back(specConstant);

        std::cout << "Specialization Constant: " << specConstant.name
                  << " (ID: " << specConstant.id
                  << ", Size: " << specConstant.size << " bytes)" << std::endl;
    }
}

std::string VulkanShaderReflection::ShaderResource::ToString() const
{
    std::ostringstream oss;
    oss << "Resource: " << name
        << " (Set: " << set
        << ", Binding: " << binding
        << ", Type: " << GetDescriptorTypeName(descriptorType)
        << ", Array Size: " << arraySize
        << ", Read Only: " << (isReadOnly ? "Yes" : "No")
        << ", Write Only: " << (isWriteOnly ? "Yes" : "No") << ")";
    return oss.str();
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

std::string VulkanShaderReflection::GetShaderStageName(VkShaderStageFlagBits stage)
{
    switch (stage)
    {
        case VK_SHADER_STAGE_VERTEX_BIT: return "Vertex";
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return "Tessellation Control";
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return "Tessellation Evaluation";
        case VK_SHADER_STAGE_GEOMETRY_BIT: return "Geometry";
        case VK_SHADER_STAGE_FRAGMENT_BIT: return "Fragment";
        case VK_SHADER_STAGE_COMPUTE_BIT: return "Compute";
        default: return "Unknown";
    }
}

std::string VulkanShaderReflection::GetDescriptorTypeName(VkDescriptorType type)
{
    switch (type)
    {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return "Uniform Buffer";
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return "Storage Buffer";
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return "Combined Image Sampler";
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return "Storage Image";
        case VK_DESCRIPTOR_TYPE_SAMPLER: return "Sampler";
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return "Sampled Image";
        default: return "Unknown";
    }
}

std::string VulkanShaderReflection::GetVkFormatName(VkFormat format)
{
	switch (format)
	{
		// 32-bit float formats
		case VK_FORMAT_R32_SFLOAT: return "R32_SFLOAT";
		case VK_FORMAT_R32G32_SFLOAT: return "R32G32_SFLOAT";
		case VK_FORMAT_R32G32B32_SFLOAT: return "R32G32B32_SFLOAT";
		case VK_FORMAT_R32G32B32A32_SFLOAT: return "R32G32B32A32_SFLOAT";

		// 32-bit signed integer formats
		case VK_FORMAT_R32_SINT: return "R32_SINT";
		case VK_FORMAT_R32G32_SINT: return "R32G32_SINT";
		case VK_FORMAT_R32G32B32_SINT: return "R32G32B32_SINT";
		case VK_FORMAT_R32G32B32A32_SINT: return "R32G32B32A32_SINT";

		// 32-bit unsigned integer formats
		case VK_FORMAT_R32_UINT: return "R32_UINT";
		case VK_FORMAT_R32G32_UINT: return "R32G32_UINT";
		case VK_FORMAT_R32G32B32_UINT: return "R32G32B32_UINT";
		case VK_FORMAT_R32G32B32A32_UINT: return "R32G32B32A32_UINT";

		// 16-bit float formats
		case VK_FORMAT_R16_SFLOAT: return "R16_SFLOAT";
		case VK_FORMAT_R16G16_SFLOAT: return "R16G16_SFLOAT";
		case VK_FORMAT_R16G16B16_SFLOAT: return "R16G16B16_SFLOAT";
		case VK_FORMAT_R16G16B16A16_SFLOAT: return "R16G16B16A16_SFLOAT";

		// 16-bit signed integer formats
		case VK_FORMAT_R16_SINT: return "R16_SINT";
		case VK_FORMAT_R16G16_SINT: return "R16G16_SINT";
		case VK_FORMAT_R16G16B16_SINT: return "R16G16B16_SINT";
		case VK_FORMAT_R16G16B16A16_SINT: return "R16G16B16A16_SINT";

		// 16-bit unsigned integer formats
		case VK_FORMAT_R16_UINT: return "R16_UINT";
		case VK_FORMAT_R16G16_UINT: return "R16G16_UINT";
		case VK_FORMAT_R16G16B16_UINT: return "R16G16B16_UINT";
		case VK_FORMAT_R16G16B16A16_UINT: return "R16G16B16A16_UINT";

		// 8-bit signed integer formats
		case VK_FORMAT_R8_SINT: return "R8_SINT";
		case VK_FORMAT_R8G8_SINT: return "R8G8_SINT";
		case VK_FORMAT_R8G8B8_SINT: return "R8G8B8_SINT";
		case VK_FORMAT_R8G8B8A8_SINT: return "R8G8B8A8_SINT";

		// 8-bit unsigned integer formats
		case VK_FORMAT_R8_UINT: return "R8_UINT";
		case VK_FORMAT_R8G8_UINT: return "R8G8_UINT";
		case VK_FORMAT_R8G8B8_UINT: return "R8G8B8_UINT";
		case VK_FORMAT_R8G8B8A8_UINT: return "R8G8B8A8_UINT";

		// 8-bit unorm formats
		case VK_FORMAT_R8_UNORM: return "R8_UNORM";
		case VK_FORMAT_R8G8_UNORM: return "R8G8_UNORM";
		case VK_FORMAT_R8G8B8_UNORM: return "R8G8B8_UNORM";
		case VK_FORMAT_R8G8B8A8_UNORM: return "R8G8B8A8_UNORM";

		// 8-bit snorm formats
		case VK_FORMAT_R8_SNORM: return "R8_SNORM";
		case VK_FORMAT_R8G8_SNORM: return "R8G8_SNORM";
		case VK_FORMAT_R8G8B8_SNORM: return "R8G8B8_SNORM";
		case VK_FORMAT_R8G8B8A8_SNORM: return "R8G8B8A8_SNORM";

		// Special formats
		case VK_FORMAT_B8G8R8A8_UNORM: return "B8G8R8A8_UNORM";
		case VK_FORMAT_B8G8R8A8_SRGB: return "B8G8R8A8_SRGB";

		default: return "Unknown Format";
	}
}

uint32_t VulkanShaderReflection::GetTotalDescriptorCountAcrossAllSets() const
{
    size_t totalElements = std::accumulate(m_DescriptorSets.begin(), m_DescriptorSets.end(), 0,
                                           [](size_t sum, const auto& pair)
                                            {
                                                return sum + pair.second.size();
                                            });
    return totalElements;
}
