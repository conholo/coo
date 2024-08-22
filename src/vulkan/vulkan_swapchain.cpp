#include "vulkan_swapchain.h"

#include "core/frame_info.h"
#include "vulkan/render_passes/render_graph.h"
#include "vulkan/render_passes/render_graph_resource_declarations.h"
#include "vulkan_context.h"
#include "vulkan_fence.h"
#include "vulkan_framebuffer.h"
#include "vulkan_render_pass.h"
#include "vulkan_texture.h"
#include "vulkan_semaphore.h"
#include "vulkan_utils.h"

#include <core/application.h>

#include <utility>

VulkanSwapchain::VulkanSwapchain(RenderGraph& graph, VkExtent2D windowExtent)
	: m_WindowExtent(windowExtent)
{
	Invalidate(graph);
}

VulkanSwapchain::VulkanSwapchain(RenderGraph& graph, VkExtent2D windowExtent, std::shared_ptr<VulkanSwapchain> previous)
	: m_WindowExtent(windowExtent), m_PreviousSwapchain{std::move(previous)}
{
	Invalidate(graph);
	if (m_PreviousSwapchain)
	{
		m_PreviousSwapchain->DestroyHandle();
		m_PreviousSwapchain = nullptr;
	}
}

VulkanSwapchain::~VulkanSwapchain()
{
	// The only resource explicitly owned by the swapchain is the VkSwapchainKHR - so free that.
	// The rest are owned by the RenderGraph instance.
	// The SwapchainRenderer needs to explicitly signal to the RenderGraph when it's time to free all
	// swapchain-related resources.
	DestroyHandle();
}

void VulkanSwapchain::DestroyHandle()
{
	auto device = VulkanContext::Get().Device();

	if (m_Swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
		m_Swapchain = VK_NULL_HANDLE;
	}
}

VkResult VulkanSwapchain::AcquireNextImage(RenderGraph& graph, uint32_t frameIndex, uint32_t* imageIndex)
{
	// For the current frame in flight, make sure that the last set of resources used (UI pass) on the swapchain image
	// has been submitted.
	// For example:
	/*
	 * Frame 1 (frame in flight 1): Acquisition - Nothing to wait for -> Passes -> UI Submission, Fence Set, Not Signaled
	 * Frame 2 (frame in flight 0): ...
	 * Frame 3 (frame in flight 1): Acquisition - Wait for fence submitted in Frame 1 before starting the new frame.
	 */
	auto* swapchainCommandBufferResource = graph.GetResource<CommandBufferResource>(SwapchainCommandBufferResourceName, frameIndex);
	auto* swapchainResourcesInFlightResource = graph.GetResource<FenceResource>(SwapchainResourcesInFlightFenceResourceName, frameIndex);
	swapchainCommandBufferResource->Get()->WaitForCompletion(swapchainResourcesInFlightResource->Get()->GetHandle());

	auto* imageAvailableSemaphoreResource = graph.GetResource<SemaphoreResource>(SwapchainImageAvailableSemaphoreResourceName, frameIndex);

	VkResult result = vkAcquireNextImageKHR(
		VulkanContext::Get().Device(),
		m_Swapchain,
		std::numeric_limits<uint64_t>::max(),
		imageAvailableSemaphoreResource->Get()->GetHandle(),
		VK_NULL_HANDLE,
		imageIndex);

	return result;
}

VkResult VulkanSwapchain::Present(RenderGraph& graph, uint32_t frameIndex, const uint32_t* imageIndex)
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	auto swapchainRenderingCompleteSemaphore = graph.GetResource<SemaphoreResource>(SwapchainRenderingCompleteSemaphoreResourceName, frameIndex);
	VkSemaphore waitSemaphores[] = { swapchainRenderingCompleteSemaphore->Get()->GetHandle() };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = waitSemaphores;

	VkSwapchainKHR swapChains[] = {m_Swapchain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = imageIndex;
	auto result = vkQueuePresentKHR(VulkanContext::Get().PresentQueue(), &presentInfo);
	return result;
}

void VulkanSwapchain::FreeAllResources(RenderGraph& graph)
{
	// Free the swapchain if it exists
	DestroyHandle();

	// Free the images if they exist
	auto freeSwapchainImages =
		[](std::shared_ptr<VulkanImage2D> image)
	{
		image->ReleaseSwapchainResources();
		image.reset();
	};
	graph.TryFreeResources<Image2DResource>(SwapchainImage2DResourceName, freeSwapchainImages);

	m_SwapchainImagesHandles.clear();
}

void VulkanSwapchain::Invalidate(RenderGraph& graph)
{
	FreeAllResources(graph);

	CreateSwapchain(graph);
	CreateImages(graph);
}

void VulkanSwapchain::CreateSwapchain(RenderGraph& graph)
{
	SwapchainSupportDetails swapChainSupport = VulkanContext::Get().QuerySwapchainSupportDetailsOnSwapchainRecreation();

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

	m_ImageCount = swapChainSupport.Capabilities.minImageCount + 1;
	if (swapChainSupport.Capabilities.maxImageCount > 0 && m_ImageCount > swapChainSupport.Capabilities.maxImageCount)
	{
		m_ImageCount = swapChainSupport.Capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = VulkanContext::Get().Surface();

	createInfo.minImageCount = m_ImageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	QueueFamilyIndices indices = VulkanContext::Get().GetAvailableDeviceQueueFamilyIndices();
	uint32_t queueFamilyIndices[] = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

	if (indices.GraphicsFamily.value() != indices.PresentFamily.value())
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;		 // Optional
		createInfo.pQueueFamilyIndices = nullptr;	 // Optional
	}

	createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = m_PreviousSwapchain == nullptr ? VK_NULL_HANDLE : m_PreviousSwapchain->m_Swapchain;

	VK_CHECK_RESULT(vkCreateSwapchainKHR(VulkanContext::Get().Device(), &createInfo, nullptr, &m_Swapchain));

	m_SwapchainImageFormat = surfaceFormat.format;
	m_SwapchainExtent = extent;
}

void VulkanSwapchain::CreateImages(RenderGraph& graph)
{
	vkGetSwapchainImagesKHR(VulkanContext::Get().Device(), m_Swapchain, &m_ImageCount, nullptr);
	std::vector<VkImage> swapChainImages(m_ImageCount);
	vkGetSwapchainImagesKHR(VulkanContext::Get().Device(), m_Swapchain, &m_ImageCount, swapChainImages.data());

	auto swapchainImageCreateFn =
		[&](size_t index, const std::string& resourceBaseName, const std::vector<VkImage>& swapchainImages)
	{
		auto resourceName = resourceBaseName + " " + std::to_string(index);;
		ImageSpecification spec;
		spec.Width = m_SwapchainExtent.width;
		spec.Height = m_SwapchainExtent.height;
		spec.Format = ImageUtils::VulkanFormatToImageFormat(m_SwapchainImageFormat);
		spec.Usage = ImageUsage::Swapchain;
		spec.ExistingImage = swapchainImages[index];
		spec.SwapchainFormat = m_SwapchainImageFormat;
		spec.CreateSampler = true;
		spec.DebugName = resourceName;

		auto image2D = std::make_shared<VulkanImage2D>(spec);
		return std::make_shared<Image2DResource>(resourceName, image2D);
	};

	m_SwapchainImagesHandles = graph.CreateResources<Image2DResource>(m_ImageCount, SwapchainImage2DResourceName, swapchainImageCreateFn, swapChainImages);
}


VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			std::cout << "Present mode: Mailbox" << std::endl;
			return availablePresentMode;
		}
	}

	std::cout << "Present mode: V-Sync" << std::endl;
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = m_WindowExtent;
		actualExtent.width =
			std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height =
			std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}
