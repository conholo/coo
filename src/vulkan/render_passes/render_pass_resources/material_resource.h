#pragma once

#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan/vulkan_material.h"

class MaterialResource : public RenderPassResource
{
public:
	explicit MaterialResource(const std::string& name)
		: RenderPassResource(name)
	{
	}

	MaterialResource(const std::string& name, VulkanMaterial* vulkanMaterial)
		: RenderPassResource(name), m_Material(vulkanMaterial)
	{
	}

	~MaterialResource()
	{
		delete m_Material;
	}

	VulkanMaterial& Get() const
	{
		return *m_Material;
	}

	template<typename... Args>
	void Create(Args&&... args)
	{
		delete m_Material;
		m_Material = new VulkanMaterial(std::forward<Args>(args)...);
	}

private:
	VulkanMaterial* m_Material = nullptr;
};
