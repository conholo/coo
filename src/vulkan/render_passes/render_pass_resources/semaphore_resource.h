#pragma once

#include "vulkan/render_passes/render_pass_resource.h"
#include "vulkan/vulkan_semaphore.h"

class SemaphoreResource : public RenderPassResource
{
public:
	explicit SemaphoreResource(const std::string& name)
		: RenderPassResource(name)
	{
	}

	SemaphoreResource(const std::string& name, VulkanSemaphore* semaphore)
		: RenderPassResource(name), m_Semaphore(semaphore)
	{
	}

	~SemaphoreResource()
	{
		delete m_Semaphore;
	}

	VulkanSemaphore& Get() const
	{
		return *m_Semaphore;
	}

	template<typename... Args>
	void Create(Args&&... args)
	{
		delete m_Semaphore;
		m_Semaphore = new VulkanSemaphore(std::forward<Args>(args)...);
	}

private:
	VulkanSemaphore* m_Semaphore = nullptr;
};
