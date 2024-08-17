#pragma once

#include "vulkan_swapchain_renderer.h"

#include "core/frame_info.h"
#include "irenderer.h"

#include <ui/imgui_vulkan_renderer.h>
#include <ui/imgui_vulkan_viewport.h>

#include <memory>

class GameObject;

class VulkanRenderer
{
public:
    explicit VulkanRenderer(Window& windowRef);
    ~VulkanRenderer() = default;

    void Initialize();
	void Render(FrameInfo& frameInfo);
    void Shutdown();
	void OnEvent(Event& event);
	void DrawUI(FrameInfo& frameInfo);

    void PrepareGameObjectForRendering(GameObject& gameObjectRef);
    uint32_t GetCurrentSwapchainImageIndex() const { return m_SwapchainRenderer->CurrentImageIndex(); }
	uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

    VulkanSwapchain& VulkanSwapchain() const { return m_SwapchainRenderer->GetSwapchain(); }
	VulkanImGuiRenderer& GetImGuiRenderer() { return *m_ImGuiRenderer; }
	VulkanImGuiViewport& GetImGuiViewport() { return *m_ImGuiViewport; }
	VkSemaphore ImageAvailableSemaphore(uint32_t frameIndex) { return m_SwapchainRenderer->GetImageAvailableSemaphore(frameIndex); }

	IRenderer& GetActiveSceneRenderer() { return *m_SceneRenderer; }
	VulkanSwapchainRenderer& GetSwapchainRenderer() { return *m_SwapchainRenderer; }

private:
    Window& m_WindowRef;

	std::unique_ptr<IRenderer> m_SceneRenderer;
    std::shared_ptr<VulkanSwapchainRenderer> m_SwapchainRenderer;
    std::unique_ptr<VulkanImGuiRenderer> m_ImGuiRenderer;
    std::unique_ptr<VulkanImGuiViewport> m_ImGuiViewport;
    std::vector<std::shared_ptr<VulkanBuffer>> m_GlobalUboBuffers{VulkanSwapchain::MAX_FRAMES_IN_FLIGHT};

	uint32_t m_CurrentFrameIndex{};
    void OnSwapchainRecreate(uint32_t width, uint32_t height);
};
