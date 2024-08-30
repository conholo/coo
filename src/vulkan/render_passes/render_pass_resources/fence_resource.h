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

	~FenceResource() override
	{
		delete m_Fence;
		m_Fence = nullptr;
	}

	VulkanFence& Get() const
	{
		return *m_Fence;
	}

	void Set(VulkanFence* fence)
	{
		delete m_Fence;
		m_Fence = fence;
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
