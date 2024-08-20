#include "vulkan_swapchain_renderer.h"

#include "vulkan_utils.h"

#include <array>
#include <stdexcept>

VulkanSwapchainRenderer::VulkanSwapchainRenderer(RenderGraph& graph, Window& windowRef)
	: m_WindowRef(windowRef)
{
	RecreateSwapchain(graph);
}

void VulkanSwapchainRenderer::Shutdown(RenderGraph& graph)
{
	if(m_Swapchain != nullptr)
	{
		m_Swapchain->FreeAllResources(graph);
		m_Swapchain = nullptr;
	}
}

void VulkanSwapchainRenderer::RecreateSwapchain(RenderGraph& graph)
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
		m_Swapchain = std::make_unique<VulkanSwapchain>(graph, extent);
	}
	else
	{
		std::shared_ptr<VulkanSwapchain> oldSwapChain = std::move(m_Swapchain);
		m_Swapchain = std::make_unique<VulkanSwapchain>(graph, extent, oldSwapChain);

		if (!oldSwapChain->CompareFormats(*m_Swapchain))
			throw std::runtime_error("Swap chain imageInfo format has changed!");
	}

	if(m_RecreateSwapchainCallback)
		m_RecreateSwapchainCallback(m_Swapchain->Width(), m_Swapchain->Height());

	vkDeviceWaitIdle(VulkanContext::Get().Device());
}

bool VulkanSwapchainRenderer::BeginFrame(RenderGraph& graph, uint32_t frameIndex)
{
	auto result = m_Swapchain->AcquireNextImage(graph, frameIndex, &m_CurrentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapchain(graph);
		return false;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image!");
	}
	return true;
}

void VulkanSwapchainRenderer::EndFrame(RenderGraph& graph, uint32_t frameIndex)
{
	auto result = m_Swapchain->Present(graph, frameIndex, &m_CurrentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_WindowRef.WasWindowResized())
	{
		m_WindowRef.ResetWindowResizedFlag();
		RecreateSwapchain(graph);
	}
	else if (result != VK_SUCCESS)
	{
		std::cout << "Presentation Result: " << VKResultToString(result) << "\n";
		throw std::runtime_error("failed to present swap chain image!");
	}
}
