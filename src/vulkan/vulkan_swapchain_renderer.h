#pragma once

#include "vulkan_swapchain.h"
#include "core/window.h"
#include <cassert>
#include <functional>

class VulkanSwapchainRenderer
{
public:
    using OnRecreateSwapchainCallbackFn = std::function<void(uint32_t, uint32_t)>;

    explicit VulkanSwapchainRenderer(Window& windowRef);
    ~VulkanSwapchainRenderer();

    VulkanSwapchainRenderer(const VulkanSwapchainRenderer &) = delete;
    VulkanSwapchainRenderer &operator=(const VulkanSwapchainRenderer &) = delete;

    void SetOnRecreateSwapchainCallback(const OnRecreateSwapchainCallbackFn &callbackFn) { m_RecreateSwapchainCallback = callbackFn; }
    VulkanSwapchain& GetSwapchain() { return *m_Swapchain; }

    VkCommandBuffer BeginFrame(uint32_t frameIndex);
    void EndFrame(uint32_t frameIndex);
    uint32_t CurrentImageIndex() const { return m_CurrentImageIndex; }

private:

    void CreateDrawCommandBuffers();
    void FreeCommandBuffers();
    void RecreateSwapchain();

    Window &m_WindowRef;
    OnRecreateSwapchainCallbackFn m_RecreateSwapchainCallback;

    std::unique_ptr<VulkanSwapchain> m_Swapchain;
    std::vector<VkCommandBuffer> m_DrawCommandBuffers;

    std::vector<VkFence> m_InFlightFences;
    std::vector<VkFence> m_ImagesInFlight;
    std::vector<VkSemaphore> m_PresentCompleteSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    uint32_t m_CurrentImageIndex{0};
};