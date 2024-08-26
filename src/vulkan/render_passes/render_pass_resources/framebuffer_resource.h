#pragma once

#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan/vulkan_framebuffer.h"

class FramebufferResource : public RenderPassResource
{
public:
	explicit FramebufferResource(const std::string& name)
		: RenderPassResource(name)
	{
	}

	FramebufferResource(const std::string& name, VulkanFramebuffer* vulkanFramebuffer)
		: RenderPassResource(name), m_Framebuffer(vulkanFramebuffer)
	{
	}

	~FramebufferResource()
	{
		delete m_Framebuffer;
	}

	VulkanFramebuffer& Get() const
	{
		return *m_Framebuffer;
	}

	template<typename... Args>
	void Create(Args&&... args)
	{
		delete m_Framebuffer;
		m_Framebuffer = new VulkanFramebuffer(std::forward<Args>(args)...);
	}

private:
	VulkanFramebuffer* m_Framebuffer = nullptr;
};
