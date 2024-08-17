#pragma once

#include "vulkan_swapchain.h"

#include "core/frame_info.h"
#include "core/window.h"

#include <cassert>
#include <functional>

class VulkanCommandBuffer;

class VulkanSwapchainRenderer
{
public:
    using OnRecreateSwapchainCallbackFn = std::function<void(uint32_t, uint32_t)>;

    explicit VulkanSwapchainRenderer(Window& windowRef);
    ~VulkanSwapchainRenderer() = default;
	void Shutdown();

    VulkanSwapchainRenderer(const VulkanSwapchainRenderer &) = delete;
    VulkanSwapchainRenderer &operator=(const VulkanSwapchainRenderer &) = delete;

    void SetOnRecreateSwapchainCallback(const OnRecreateSwapchainCallbackFn &callbackFn) { m_RecreateSwapchainCallback = callbackFn; }

    VulkanSwapchain& GetSwapchain() { return *m_Swapchain; }
	VkSemaphore GetImageAvailableSemaphore(uint32_t index) { return m_Swapchain->m_ImageAvailableSemaphores[index]; }
	uint32_t CurrentImageIndex() const { return m_CurrentImageIndex; }
	std::weak_ptr<VulkanCommandBuffer> GetDrawCmdBuffer(uint32_t frameIndex) const { return m_DrawCommandBuffers[frameIndex]; }

	std::weak_ptr<VulkanCommandBuffer> BeginFrame(uint32_t frameIndex);
	void EndFrame(FrameInfo& frameInfo);

private:

    void CreateDrawCommandBuffers();
    void RecreateSwapchain();

    Window& m_WindowRef;
    OnRecreateSwapchainCallbackFn m_RecreateSwapchainCallback;
	std::vector<VkSemaphore> m_RenderCompleteSemaphores;

    std::unique_ptr<VulkanSwapchain> m_Swapchain;
    std::vector<std::shared_ptr<VulkanCommandBuffer>> m_DrawCommandBuffers;
    uint32_t m_CurrentImageIndex{0};

	friend class VulkanRenderer;
};