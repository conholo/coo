#pragma once

#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan/vulkan_shader.h"

class ShaderResource : public RenderPassResource
{
public:
	explicit ShaderResource(const std::string& name)
		: RenderPassResource(name)
	{
	}

	ShaderResource(const std::string& name, VulkanShader* shader)
		: RenderPassResource(name), m_Shader(shader)
	{
	}

	~ShaderResource()
	{
		delete m_Shader;
	}

	VulkanShader& Get() const
	{
		return *m_Shader;
	}

	template<typename... Args>
	void Create(Args&&... args)
	{
		delete m_Shader;
		m_Shader = new VulkanShader(std::forward<Args>(args)...);
	}

private:
	VulkanShader* m_Shader = nullptr;
};
