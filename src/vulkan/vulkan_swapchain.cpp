#include "vulkan_swapchain.h"

#include "core/frame_info.h"
#include "vulkan_context.h"
#include "vulkan_utils.h"

#include <utility>

VulkanSwapchain::VulkanSwapchain(VkExtent2D windowExtent) : m_WindowExtent(windowExtent)
{
	Initialize();
}

VulkanSwapchain::VulkanSwapchain(VkExtent2D windowExtent, std::shared_ptr<VulkanSwapchain> previous)
	: m_WindowExtent(windowExtent), m_PreviousSwapchain{std::move(previous)}
{
	Initialize();
	if (m_PreviousSwapchain)
	{
		m_PreviousSwapchain->Destroy();
		m_PreviousSwapchain = nullptr;
	}
}

VulkanSwapchain::~VulkanSwapchain()
{
	Destroy();
}

void VulkanSwapchain::Destroy()
{
	auto device = VulkanContext::Get().Device();

	for (auto& image : m_Images)
	{
		image->ReleaseSwapchainResources();
	}
	m_Images.clear();

	if (m_Swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
		m_Swapchain = VK_NULL_HANDLE;
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (m_ImageAvailableSemaphores[i] != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
			m_ImageAvailableSemaphores[i] = VK_NULL_HANDLE;
		}
		if (m_RenderCompleteSemaphores[i] != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(device, m_RenderCompleteSemaphores[i], nullptr);
			m_RenderCompleteSemaphores[i] = VK_NULL_HANDLE;
		}
		if (m_InFlightFences[i] != VK_NULL_HANDLE)
		{
			vkDestroyFence(device, m_InFlightFences[i], nullptr);
			m_InFlightFences[i] = VK_NULL_HANDLE;
		}
	}
	m_ImagesInFlight.clear();
}

void VulkanSwapchain::Initialize()
{
	CreateSwapchain();
	CreateSyncObjects();
}

void VulkanSwapchain::CreateSwapchain()
{
	SwapchainSupportDetails swapChainSupport = VulkanContext::Get().QuerySwapchainSupportDetailsOnSwapchainRecreation();

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

	uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
	if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.Capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = VulkanContext::Get().Surface();

	createInfo.minImageCount = imageCount;
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

	vkGetSwapchainImagesKHR(VulkanContext::Get().Device(), m_Swapchain, &imageCount, nullptr);
	std::vector<VkImage> swapChainImages(imageCount);
	vkGetSwapchainImagesKHR(VulkanContext::Get().Device(), m_Swapchain, &imageCount, swapChainImages.data());

	m_Images.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; i++)
	{
		ImageSpecification spec;
		spec.Width = m_SwapchainExtent.width;
		spec.Height = m_SwapchainExtent.height;
		spec.Format = ImageUtils::VulkanFormatToImageFormat(m_SwapchainImageFormat);
		spec.Usage = ImageUsage::Swapchain;
		spec.ExistingImage = swapChainImages[i];
		spec.SwapchainFormat = m_SwapchainImageFormat;
		spec.CreateSampler = true;
		spec.DebugName = "Swapchain Image " + std::to_string(i);
		m_Images[i] = std::make_shared<VulkanImage2D>(spec);
	}
}

VkResult VulkanSwapchain::AcquireNextImage(uint32_t frameIndex, uint32_t* imageIndex)
{
	vkWaitForFences(VulkanContext::Get().Device(), 1, &m_InFlightFences[frameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());

	VkResult result = vkAcquireNextImageKHR(VulkanContext::Get().Device(), m_Swapchain, std::numeric_limits<uint64_t>::max(),
		m_ImageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, imageIndex);

	return result;
}

void VulkanSwapchain::SubmitCommandBuffer(FrameInfo& frameInfo)
{
	/*
	 *  Before submitting the composition/display command buffer, wait on the pass before this using the semaphore.
	 *  Let the renderer decide on the exact barrier - in the case of a single pass renderer, the wait condition for the
	 *  swapchain draw submission is for the next swapchain image to become available.  The signal condition provided by the
	 * 	renderer is *usually* the only wait condition needed for displaying the image. For the case of a more involved multi-pass
	 * 	renderer, the wait condition for the swapchain command buffer submission will be the previous pass in the chain of passes.
	 */

	// Submit Composition pass
	std::vector<VkSemaphore> waitSemaphores = { frameInfo.RendererCompleteSemaphore };
	std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkSemaphore> signalSemaphores = { m_RenderCompleteSemaphores[frameInfo.FrameIndex] };
	std::vector<VulkanCommandBuffer*> drawCmdBuffers = { &*frameInfo.SwapchainSubmitCommandBuffer.lock() };

	VulkanCommandBuffer::Submit(
		VulkanContext::Get().GraphicsQueue(),
		drawCmdBuffers,
		waitSemaphores,
		waitStages,
		signalSemaphores);
}

VkResult VulkanSwapchain::Present(FrameInfo& frameInfo, const uint32_t* imageIndex)
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	VkSemaphore waitSemaphores[] = { m_RenderCompleteSemaphores[frameInfo.FrameIndex] };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = waitSemaphores;

	VkSwapchainKHR swapChains[] = {m_Swapchain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = imageIndex;
	auto result = vkQueuePresentKHR(VulkanContext::Get().PresentQueue(), &presentInfo);
	return result;
}

std::weak_ptr<VulkanImage2D> VulkanSwapchain::GetImage(uint32_t index) const
{
	if (index >= m_Images.size())
		return {};
	return m_Images[index];
}

void VulkanSwapchain::CreateSyncObjects()
{
	auto device = VulkanContext::Get().Device();
	m_ImageAvailableSemaphores.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	m_RenderCompleteSemaphores.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	m_InFlightFences.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	m_ImagesInFlight.resize(ImageCount(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]));
		auto semaphoreName = std::string("Image Available ") + std::to_string(i);
		SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t) m_ImageAvailableSemaphores[i], semaphoreName.c_str());

		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderCompleteSemaphores[i]));
		semaphoreName = std::string("Render Complete ") + std::to_string(i);
		SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t) m_RenderCompleteSemaphores[i], semaphoreName.c_str());

		VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &m_InFlightFences[i]));
		auto fenceName = std::string("In Flight Fence ") + std::to_string(i);
		SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_FENCE, (uint64_t) m_InFlightFences[i], fenceName.c_str());
	}
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
