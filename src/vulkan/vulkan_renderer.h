#pragma once

#include "vulkan_swapchain_renderer.h"

#include "core/frame_info.h"
#include "irenderer.h"

#include "ui/imgui_vulkan_renderer.h"
#include "ui/imgui_vulkan_viewport.h"
#include "render_passes/render_graph.h"

#include <memory>

class GameObject;

class VulkanRenderer
{
public:
    VulkanRenderer(Window& windowRef, RenderGraph& graph);
    ~VulkanRenderer() = default;

    void Initialize();
	void Render(FrameInfo& frameInfo);
    void Shutdown();
	void OnEvent(Event& event);

    void PrepareGameObjectForRendering(GameObject& gameObjectRef);
    uint32_t GetCurrentSwapchainImageIndex() const { return m_SwapchainRenderer->CurrentImageIndex(); }
	uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

    VulkanSwapchain& VulkanSwapchain() const { return m_SwapchainRenderer->GetSwapchain(); }
	VulkanSwapchainRenderer& GetSwapchainRenderer() { return *m_SwapchainRenderer; }

private:
	RenderGraph& m_GraphRef;
    Window& m_WindowRef;
    std::shared_ptr<VulkanSwapchainRenderer> m_SwapchainRenderer;

	uint32_t m_CurrentFrameIndex{};
    void OnSwapchainRecreate(uint32_t width, uint32_t height);
};
