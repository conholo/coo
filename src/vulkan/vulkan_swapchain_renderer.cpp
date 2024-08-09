#include "vulkan_swapchain_renderer.h"

#include "vulkan_utils.h"

#include <array>
#include <stdexcept>

VulkanSwapchainRenderer::VulkanSwapchainRenderer(Window& windowRef) : m_WindowRef(windowRef)
{
	CreateDrawCommandBuffers();
	RecreateSwapchain();
}

VulkanSwapchainRenderer::~VulkanSwapchainRenderer()
{
	FreeCommandBuffers();
}

void VulkanSwapchainRenderer::RecreateSwapchain()
{
	auto extent = m_WindowRef.GetExtent();
	while (extent.width == 0 || extent.height == 0)
	{
		extent = m_WindowRef.GetExtent();
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(VulkanContext::Get().Device());

	if (m_Swapchain == nullptr)
	{
		m_Swapchain = std::make_unique<VulkanSwapchain>(extent);
	}
	else
	{
		std::shared_ptr<VulkanSwapchain> oldSwapChain = std::move(m_Swapchain);
		m_Swapchain = std::make_unique<VulkanSwapchain>(extent, oldSwapChain);

		if (!oldSwapChain->CompareFormats(*m_Swapchain))
			throw std::runtime_error("Swap chain imageInfo format has changed!");
	}

	FreeCommandBuffers();
	CreateDrawCommandBuffers();
	if(m_RecreateSwapchainCallback)
		m_RecreateSwapchainCallback(m_Swapchain->Width(), m_Swapchain->Height());

	vkDeviceWaitIdle(VulkanContext::Get().Device());
}

void VulkanSwapchainRenderer::CreateDrawCommandBuffers()
{
	m_DrawCommandBuffers.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = VulkanContext::Get().GraphicsCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_DrawCommandBuffers.size());

	if (vkAllocateCommandBuffers(VulkanContext::Get().Device(), &allocInfo, m_DrawCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers!");
	}
}

void VulkanSwapchainRenderer::FreeCommandBuffers()
{
	vkFreeCommandBuffers(
		VulkanContext::Get().Device(),
		VulkanContext::Get().GraphicsCommandPool(),
		static_cast<uint32_t>(m_DrawCommandBuffers.size()),
		m_DrawCommandBuffers.data());
	m_DrawCommandBuffers.clear();
}

VkCommandBuffer VulkanSwapchainRenderer::BeginFrame(uint32_t frameIndex)
{
	auto result = m_Swapchain->AcquireNextImage(frameIndex, &m_CurrentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapchain();
		return VK_NULL_HANDLE;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image!");
	}

	vkResetFences(VulkanContext::Get().Device(), 1, &m_Swapchain->m_InFlightFences[frameIndex]);
	vkResetCommandBuffer(m_DrawCommandBuffers[frameIndex], 0);
	return m_DrawCommandBuffers[frameIndex];
}

void VulkanSwapchainRenderer::EndFrame(FrameInfo& frameInfo)
{
	m_Swapchain->SubmitCommandBuffer(frameInfo);
	auto result = m_Swapchain->Present(frameInfo, &m_CurrentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_WindowRef.WasWindowResized())
	{
		m_WindowRef.ResetWindowResizedFlag();
		RecreateSwapchain();
	}
	else if (result != VK_SUCCESS)
	{
		std::cout << "Presentation Result: " << VKResultToString(result) << "\n";
		throw std::runtime_error("failed to present swap chain image!");
	}
}
