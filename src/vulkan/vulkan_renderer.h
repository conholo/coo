#pragma once

#include "vulkan_swapchain_renderer.h"
#include "vulkan_deferred_renderer.h"
#include "core/game_object.h"
#include "core/frame_info.h"

#include <memory>

class VulkanRenderer
{
public:
    explicit VulkanRenderer(Window& windowRef);
    ~VulkanRenderer();

    void Initialize();
    void Render(FrameInfo& frameInfo);
    void Shutdown();

    void PrepareGameObjectForRendering(GameObject& gameObjectRef);
    uint32_t GetCurrentSwapchainImageIndex() const { return m_SwapchainRenderer->CurrentImageIndex(); }

    VulkanSwapchain& VulkanSwapchain() const { return m_SwapchainRenderer->GetSwapchain(); }

private:
    Window& m_WindowRef;
    std::unique_ptr<VulkanSwapchainRenderer> m_SwapchainRenderer;
    std::unique_ptr<VulkanDeferredRenderer> m_DeferredRenderer;
    std::vector<std::shared_ptr<VulkanBuffer>> m_GlobalUboBuffers{VulkanSwapchain::MAX_FRAMES_IN_FLIGHT};


    void OnSwapchainRecreate(uint32_t width, uint32_t height);
};
