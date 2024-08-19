#pragma once

#include <vulkan/vulkan.h>
#include <string>

class VulkanFence
{
public:
	explicit VulkanFence(std::string debugName = "Fence", bool signaled = false);
	~VulkanFence();

	// Disable copying
	VulkanFence(const VulkanFence&) = delete;
	VulkanFence& operator=(const VulkanFence&) = delete;

	// Enable moving
	VulkanFence(VulkanFence&& other) noexcept;
	VulkanFence& operator=(VulkanFence&& other) noexcept;

	VkFence GetHandle() const { return m_Fence; }

private:
	VkFence m_Fence{VK_NULL_HANDLE};
	std::string m_DebugName;
};