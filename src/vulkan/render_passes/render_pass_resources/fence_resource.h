#pragma once

#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan/vulkan_fence.h"

class FenceResource : public RenderPassResource
{
public:
	explicit FenceResource(const std::string& name)
		: RenderPassResource(name)
	{
	}

	FenceResource(const std::string& name, VulkanFence* fence)
		: RenderPassResource(name), m_Fence(fence)
	{
	}

	~FenceResource()
	{
		delete m_Fence;
	}

	VulkanFence& Get() const
	{
		return *m_Fence;
	}

	template<typename... Args>
	void Create(Args&&... args)
	{
		delete m_Fence;
		m_Fence = new VulkanFence(std::forward<Args>(args)...);
	}

private:
	VulkanFence* m_Fence = nullptr;
};
