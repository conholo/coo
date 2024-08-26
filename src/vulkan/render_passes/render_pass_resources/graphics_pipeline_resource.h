#pragma once

#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan/vulkan_graphics_pipeline.h"

class GraphicsPipelineResource : public RenderPassResource
{
public:
	explicit GraphicsPipelineResource(const std::string& name)
		: RenderPassResource(name)
	{
	}

	GraphicsPipelineResource(const std::string& name, VulkanGraphicsPipeline* graphicsPipeline)
		: RenderPassResource(name), m_GraphicsPipeline(graphicsPipeline)
	{
	}

	~GraphicsPipelineResource()
	{
		delete m_GraphicsPipeline;
	}

	VulkanGraphicsPipeline& Get() const
	{
		return *m_GraphicsPipeline;
	}

	template<typename... Args>
	void Create(Args&&... args)
	{
		delete m_GraphicsPipeline;
		m_GraphicsPipeline = new VulkanGraphicsPipeline(std::forward<Args>(args)...);
	}

private:
	VulkanGraphicsPipeline* m_GraphicsPipeline = nullptr;
};
