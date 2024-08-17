#include "vulkan_semaphore.h"
#include "vulkan_context.h"
#include "vulkan_utils.h"

VulkanSemaphore::VulkanSemaphore(std::string debugName)
	: m_DebugName(std::move(debugName))
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VK_CHECK_RESULT(vkCreateSemaphore(VulkanContext::Get().Device(), &semaphoreInfo, nullptr, &m_Semaphore));
}

VulkanSemaphore::~VulkanSemaphore()
{
	if (m_Semaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(VulkanContext::Get().Device(), m_Semaphore, nullptr);
	}
}

VulkanSemaphore::VulkanSemaphore(VulkanSemaphore&& other) noexcept
	: m_Semaphore(other.m_Semaphore), m_DebugName(std::move(other.m_DebugName))
{
	other.m_Semaphore = VK_NULL_HANDLE;
}

VulkanSemaphore& VulkanSemaphore::operator=(VulkanSemaphore&& other) noexcept
{
	if (this != &other)
	{
		if (m_Semaphore != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(VulkanContext::Get().Device(), m_Semaphore, nullptr);
		}

		m_Semaphore = other.m_Semaphore;
		m_DebugName = std::move(other.m_DebugName);

		other.m_Semaphore = VK_NULL_HANDLE;
	}
	return *this;
}