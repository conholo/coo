#pragma once

#include "vulkan_swapchain_renderer.h"
#include "vulkan_deferred_renderer.h"

#include <memory>

class VulkanRenderer
{
public:
    explicit VulkanRenderer(Window& windowRef);
    ~VulkanRenderer();

    void Initialize();
    void Render();
    void Shutdown();

    VulkanSwapchain& VulkanSwapchain() const { return m_SwapchainRenderer->GetSwapchain(); }

private:
    Window& m_WindowRef;
    std::unique_ptr<VulkanSwapchainRenderer> m_SwapchainRenderer;
    std::unique_ptr<VulkanDeferredRenderer> m_DeferredRenderer;

    void OnSwapchainRecreate(uint32_t width, uint32_t height);

    uint32_t m_FrameCounter{};
    uint32_t m_FrameIndex{};
};
