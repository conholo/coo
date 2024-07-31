#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include "vulkan_shader_reflection.h"

enum class ShaderType
{
    Vertex,
    Fragment,
    Compute,
    Geometry,
    TessellationControl,
    TessellationEvaluation
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