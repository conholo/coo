#pragma once

#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan/vulkan_buffer.h"

class BufferResource : public RenderPassResource
{
public:
	explicit BufferResource(const std::string& name)
		: RenderPassResource(name)
	{
	}

	BufferResource(const std::string& name, VulkanBuffer* buffer)
		: RenderPassResource(name), m_Buffer(buffer)
	{
	}

	~BufferResource()
	{
		delete m_Buffer;
	}

	VulkanBuffer& Get() const
	{
		return *m_Buffer;
	}

	template<typename... Args>
	void Create(Args&&... args)
	{
		delete m_Buffer;
		m_Buffer = new VulkanBuffer(std::forward<Args>(args)...);
	}

private:
	VulkanBuffer* m_Buffer = nullptr;
};
