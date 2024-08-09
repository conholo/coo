#pragma once

#include "vulkan_swapchain_renderer.h"
#include "core/game_object.h"
#include "irenderer.h"
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
	uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

    VulkanSwapchain& VulkanSwapchain() const { return m_SwapchainRenderer->GetSwapchain(); }
	VkSemaphore ImageAvailableSemaphore(uint32_t frameIndex) { return m_SwapchainRenderer->GetImageAvailableSemaphore(frameIndex); }

private:
    Window& m_WindowRef;

	std::unique_ptr<IRenderer> m_Renderer;
    std::unique_ptr<VulkanSwapchainRenderer> m_SwapchainRenderer;
    std::vector<std::shared_ptr<VulkanBuffer>> m_GlobalUboBuffers{VulkanSwapchain::MAX_FRAMES_IN_FLIGHT};

	uint32_t m_CurrentFrameIndex{};
    void OnSwapchainRecreate(uint32_t width, uint32_t height);
};
