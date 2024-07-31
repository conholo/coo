#include "vulkan_shader.h"
#include "vulkan_context.h"
#include <fstream>
#include <stdexcept>

VulkanShader::VulkanShader(std::string filePath, ShaderType type)
    : m_FilePath(std::move(filePath)), m_Type(type)
{
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
    m_ShaderCode = ReadFile(m_FilePath);
    CreateShaderModule();
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

void VulkanShader::CreateShaderModule()
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = m_ShaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(m_ShaderCode.data());

    if (vkCreateShaderModule(VulkanContext::Get().Device(), &createInfo, nullptr, &m_ShaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module!");
    }
}

std::vector<char> VulkanShader::ReadFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

void VulkanShader::PerformReflection()
{

}
