#pragma once

#include "vulkan_swapchain.h"

#include "core/frame_info.h"
#include "core/window.h"
#include "render_passes/render_graph.h"

#include <cassert>
#include <functional>

class VulkanCommandBuffer;

class VulkanSwapchainRenderer
{
public:
    using OnRecreateSwapchainCallbackFn = std::function<void(uint32_t, uint32_t)>;

    explicit VulkanSwapchainRenderer(RenderGraph& graph, Window& windowRef);
    ~VulkanSwapchainRenderer() = default;
	void Shutdown(RenderGraph& graph);

    VulkanSwapchainRenderer(const VulkanSwapchainRenderer &) = delete;
    VulkanSwapchainRenderer &operator=(const VulkanSwapchainRenderer &) = delete;

    void SetOnRecreateSwapchainCallback(const OnRecreateSwapchainCallbackFn &callbackFn) { m_RecreateSwapchainCallback = callbackFn; }
    VulkanSwapchain& GetSwapchain() { return *m_Swapchain; }
	uint32_t CurrentImageIndex() const { return m_CurrentImageIndex; }

	bool BeginFrame(RenderGraph& graph, uint32_t frameIndex);
	void EndFrame(RenderGraph& graph, uint32_t frameIndex);

private:
    void RecreateSwapchain(RenderGraph& graph);

    Window& m_WindowRef;
    OnRecreateSwapchainCallbackFn m_RecreateSwapchainCallback;

    std::unique_ptr<VulkanSwapchain> m_Swapchain;
    uint32_t m_CurrentImageIndex{0};

	friend class VulkanRenderer;
};