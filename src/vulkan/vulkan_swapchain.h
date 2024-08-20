#pragma once

#include "render_passes/render_pass.h"
#include <memory>
#include <vector>

class FrameInfo;
class RenderGraph;
class VulkanSwapchain
{
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    explicit VulkanSwapchain(RenderGraph& graph, VkExtent2D windowExtent);
    VulkanSwapchain(RenderGraph& graph, VkExtent2D windowExtent, std::shared_ptr<VulkanSwapchain> previous);
    VulkanSwapchain(const VulkanSwapchain &) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain &) = delete;
    ~VulkanSwapchain();

    VkResult AcquireNextImage(RenderGraph& graph, uint32_t frameIndex, uint32_t* imageIndex);
	VkResult Present(RenderGraph& graph, uint32_t frameIndex, const uint32_t* imageIndex);

    VkSwapchainKHR GetHandle() const { return m_Swapchain; }
    uint32_t Width() const { return m_SwapchainExtent.width; }
    uint32_t Height() const { return m_SwapchainExtent.height; }
    uint32_t ImageCount() const { return m_ImageCount; }
    VkFormat SwapchainImageFormat() const { return m_SwapchainImageFormat; }
    VkExtent2D Extent() const { return m_SwapchainExtent; }

    bool CompareFormats(const VulkanSwapchain& swapchain) const
    {
        return swapchain.m_SwapchainImageFormat == m_SwapchainImageFormat;
    }
	
private:
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

	void DestroyHandle();
	void FreeAllResources(RenderGraph& graph);
	void Invalidate(RenderGraph& graph);

	void CreateSwapchain(RenderGraph& graph);
	void CreateImages(RenderGraph& graph);

	std::vector<ResourceHandle<Image2DResource>> m_SwapchainImagesHandles;

    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    std::shared_ptr<VulkanSwapchain> m_PreviousSwapchain;
	uint32_t m_ImageCount = 0;

    VkFormat m_SwapchainImageFormat{};
    VkExtent2D m_SwapchainExtent{};
    VkExtent2D m_WindowExtent{};

	friend class VulkanSwapchainRenderer;
};