#pragma once

#include "vulkan_image.h"

#include <memory>
#include <vector>

class FrameInfo;
class VulkanSwapchain
{
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    explicit VulkanSwapchain(VkExtent2D windowExtent);
    VulkanSwapchain(VkExtent2D windowExtent, std::shared_ptr<VulkanSwapchain> previous);

    VulkanSwapchain(const VulkanSwapchain &) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain &) = delete;

    ~VulkanSwapchain();

    VkResult AcquireNextImage(uint32_t frameIndex, uint32_t* imageIndex);
	void SubmitCommandBuffer(FrameInfo& frameInfo);
	VkResult Present(FrameInfo& frameInfo, const uint32_t* imageIndex);

    VkSwapchainKHR Swapchain() const { return m_Swapchain; }
    uint32_t Width() const { return m_SwapchainExtent.width; }
    uint32_t Height() const { return m_SwapchainExtent.height; }
    uint32_t ImageCount() const { return static_cast<uint32_t>(m_Images.size()); }
    VkFormat SwapchainImageFormat() const { return m_SwapchainImageFormat; }
    VkExtent2D Extent() const { return m_SwapchainExtent; }
	std::weak_ptr<VulkanImage2D> GetImage(uint32_t index) const;

    bool CompareFormats(const VulkanSwapchain& swapchain) const
    {
        return swapchain.m_SwapchainImageFormat == m_SwapchainImageFormat;
    }

private:
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

	void Destroy();
	void Initialize();
	void CreateSwapchain();
	void CreateSyncObjects();

	std::vector<VkFence> m_InFlightFences;
	std::vector<VkFence> m_ImagesInFlight;
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderCompleteSemaphores;

    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    std::shared_ptr<VulkanSwapchain> m_PreviousSwapchain;
    std::vector<std::shared_ptr<VulkanImage2D>> m_Images;

    VkFormat m_SwapchainImageFormat{};
    VkExtent2D m_SwapchainExtent{};
    VkExtent2D m_WindowExtent{};

	friend class VulkanSwapchainRenderer;
};