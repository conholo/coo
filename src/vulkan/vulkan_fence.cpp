#include "vulkan_fence.h"
#include "vulkan_context.h"
#include "vulkan_utils.h"

VulkanFence::VulkanFence(std::string debugName, bool signaled)
	: m_DebugName(std::move(debugName))
{
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = signaled & VK_FENCE_CREATE_SIGNALED_BIT;

	VK_CHECK_RESULT(vkCreateFence(VulkanContext::Get().Device(), &fenceInfo, nullptr, &m_Fence));
	SetDebugUtilsObjectName(VulkanContext::Get().Device(), VK_OBJECT_TYPE_FENCE, (uint64_t)m_Fence, m_DebugName.c_str());
}

VulkanFence::~VulkanFence()
{
	if (m_Fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(VulkanContext::Get().Device(), m_Fence, nullptr);
	}
}

VulkanFence::VulkanFence(VulkanFence&& other) noexcept
	: m_Fence(other.m_Fence), m_DebugName(std::move(other.m_DebugName))
{
	other.m_Fence = VK_NULL_HANDLE;
}

VulkanFence& VulkanFence::operator=(VulkanFence&& other) noexcept
{
	if (this != &other)
	{
		if (m_Fence != VK_NULL_HANDLE)
		{
			vkDestroyFence(VulkanContext::Get().Device(), m_Fence, nullptr);
		}

		m_Fence = other.m_Fence;
		m_DebugName = std::move(other.m_DebugName);

		other.m_Fence = VK_NULL_HANDLE;
	}
	return *this;
}