#pragma once

#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan/vulkan_command_buffer.h"

class CommandBufferResource : public RenderPassResource
{
public:
	explicit CommandBufferResource(const std::string& name)
		: RenderPassResource(name)
	{
	}

	CommandBufferResource(const std::string& name, VulkanCommandBuffer* commandBuffer)
		: RenderPassResource(name), m_CommandBuffer(commandBuffer)
	{
	}

	~CommandBufferResource()
	{
		delete m_CommandBuffer;
	}

	VulkanCommandBuffer& Get() const
	{
		return *m_CommandBuffer;
	}

	template<typename... Args>
	void Create(Args&&... args)
	{
		delete m_CommandBuffer;
		m_CommandBuffer = new VulkanCommandBuffer(std::forward<Args>(args)...);
	}

private:
	VulkanCommandBuffer* m_CommandBuffer = nullptr;
};
