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
    void Load();

    const VulkanShaderReflection &GetReflection() const { return m_Reflection; }
    VkShaderModule GetShaderModule() const { return m_ShaderModule; }
    VkShaderStageFlagBits GetShaderStage() const;
private:

    VulkanShaderReflection m_Reflection;
    void PerformReflection();

    std::string m_FilePath;
    ShaderType m_Type;
    VkShaderModule m_ShaderModule = VK_NULL_HANDLE;
    std::vector<char> m_ShaderCode;

    void CreateShaderModule();
    std::vector<char> ReadFile(const std::string &filename);
};