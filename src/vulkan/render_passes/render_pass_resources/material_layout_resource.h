#pragma once

#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan/vulkan_material_layout.h"

class MaterialLayoutResource : public RenderPassResource
{
public:
	explicit MaterialLayoutResource(const std::string& name)
		: RenderPassResource(name)
	{
	}

	MaterialLayoutResource(const std::string& name, VulkanMaterialLayout* vulkanMaterialLayout)
		: RenderPassResource(name), m_MaterialLayout(vulkanMaterialLayout)
	{
	}

	~MaterialLayoutResource()
	{
		delete m_MaterialLayout;
	}

	VulkanMaterialLayout& Get() const
	{
		return *m_MaterialLayout;
	}

	template<typename... Args>
	void Create(Args&&... args)
	{
		delete m_MaterialLayout;
		m_MaterialLayout = new VulkanMaterialLayout(std::forward<Args>(args)...);
	}

private:
	VulkanMaterialLayout* m_MaterialLayout = nullptr;
};
