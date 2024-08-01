#include "vulkan_shader.h"
#include "vulkan_context.h"
#include <fstream>
#include <stdexcept>
#include <shaderc/shaderc.hpp>

VulkanShader::VulkanShader(std::string filePath, ShaderType type)
    : m_FilePath(std::move(filePath)), m_Type(type)
{
    Load();
    std::vector<uint32_t> byteCode = Compile();
    m_Reflection = std::make_shared<VulkanShaderReflection>(byteCode, GetShaderStage());
    CreateShaderModule(byteCode);
}

VulkanShader::~VulkanShader()
{
    if (m_ShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(VulkanContext::Get().Device(), m_ShaderModule, nullptr);
    }
}

void VulkanShader::Load()
{
    std::ifstream file(m_FilePath, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + m_FilePath);

    size_t fileSize = (size_t)file.tellg();
    m_ShaderSource.resize(fileSize);

    file.seekg(0);
    file.read(m_ShaderSource.data(), fileSize);
    file.close();
}

std::vector<uint32_t> VulkanShader::Compile()
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    // Determine shader kind
    shaderc_shader_kind kind;
    switch (m_Type)
    {
        case ShaderType::Vertex: kind = shaderc_vertex_shader; break;
        case ShaderType::Fragment: kind = shaderc_fragment_shader; break;
        case ShaderType::Compute: kind = shaderc_compute_shader; break;
        default: throw std::runtime_error("Unsupported shader type");
    }

    // Compile the shader
    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(m_ShaderSource, kind, m_FilePath.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw std::runtime_error(result.GetErrorMessage());
    }

    return {result.cbegin(), result.cend()};
}

void VulkanShader::CreateShaderModule(const std::vector<uint32_t>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    if (vkCreateShaderModule(VulkanContext::Get().Device(), &createInfo, nullptr, &m_ShaderModule) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module!");
}

VkShaderStageFlagBits VulkanShader::GetShaderStage() const
{
    switch (m_Type)
    {
        case ShaderType::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderType::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderType::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
        case ShaderType::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
        case ShaderType::TessellationControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case ShaderType::TessellationEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        default:
            throw std::runtime_error("Unknown shader type");
    }
}
