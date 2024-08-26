#pragma once

#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan/vulkan_render_pass.h"

class RenderPassObjectResource : public RenderPassResource
{
public:
	explicit RenderPassObjectResource(const std::string& name)
		: RenderPassResource(name)
	{
	}

	RenderPassObjectResource(const std::string& name, VulkanRenderPass* vulkanRenderPass)
		: RenderPassResource(name), m_RenderPass(vulkanRenderPass)
	{
	}

	~RenderPassObjectResource()
	{
		delete m_RenderPass;
	}

	VulkanRenderPass& Get() const
	{
		return *m_RenderPass;
	}

	template<typename... Args>
	void Create(Args&&... args)
	{
		delete m_RenderPass;
		m_RenderPass = new VulkanRenderPass(std::forward<Args>(args)...);
	}

private:
	VulkanRenderPass* m_RenderPass = nullptr;
};
