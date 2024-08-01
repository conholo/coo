#pragma once

#include "vulkan_image.h"
#include <vector>
#include <memory>

class VulkanSwapchain
{
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    explicit VulkanSwapchain(VkExtent2D windowExtent);
    VulkanSwapchain(VkExtent2D windowExtent, std::shared_ptr<VulkanSwapchain> previous);
    ~VulkanSwapchain();

    void Create();
    void Destroy();

    void Resize(uint32_t width, uint32_t height);

    VkResult AcquireNextImage(uint32_t *imageIndex, VkSemaphore imageAvailableSemaphore);
    void Present(VkSemaphore renderCompleteSemaphore);

    VkFormat ImageFormat() const { return m_ImageFormat; }
    VkExtent2D Extent() const { return m_Extent; }

    VkSwapchainKHR Swapchain() const { return m_Swapchain; }
    uint32_t Width() const { return m_SwapchainExtent.width; }
    uint32_t Height() const { return m_SwapchainExtent.height; }
    uint32_t ImageCount() const { return static_cast<uint32_t>(m_Images.size()); }

    bool CompareFormats(const VulkanSwapchain& swapchain) const
    {
        return  swapchain.m_SwapchainImageFormat == m_SwapchainImageFormat;
    }

    const std::shared_ptr<VulkanImage2D>& GetImage(uint32_t index) const { return m_Images[index]; }

private:
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    std::shared_ptr<VulkanSwapchain> m_PreviousSwapchain;
    std::vector<std::shared_ptr<VulkanImage2D>> m_Images;

    VkFormat m_ImageFormat{};
    VkExtent2D m_Extent{};
    VkPresentModeKHR m_PresentMode{};

    VkFormat m_SwapchainImageFormat{};
    VkExtent2D m_SwapchainExtent{};
    VkExtent2D m_WindowExtent{};

    uint32_t m_CurrentImageIndex = 0;
};