#pragma once

#include <vulkan/vulkan.h>
#include <string>

class VulkanSemaphore
{
public:
	explicit VulkanSemaphore(std::string debugName = "Semaphore");
	~VulkanSemaphore();

	// Disable copying
	VulkanSemaphore(const VulkanSemaphore&) = delete;
	VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;

	// Enable moving
	VulkanSemaphore(VulkanSemaphore&& other) noexcept;
	VulkanSemaphore& operator=(VulkanSemaphore&& other) noexcept;

	VkSemaphore GetHandle() const { return m_Semaphore; }

private:
	VkSemaphore m_Semaphore{VK_NULL_HANDLE};
	std::string m_DebugName;
};