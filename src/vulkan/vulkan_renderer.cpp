#include "vulkan_renderer.h"
#include <memory>

VulkanRenderer::VulkanRenderer(Window &window)
    : m_WindowRef(window)
{
    m_SwapchainRenderer = std::make_unique<VulkanSwapchainRenderer>(window);
    m_SwapchainRenderer->SetOnRecreateSwapchainCallback(
        [this](uint32_t width, uint32_t height)
                {
                    OnSwapchainRecreate(width, height);
                });
    m_DeferredRenderer = std::make_unique<VulkanDeferredRenderer>(this);
}

VulkanRenderer::~VulkanRenderer()
{

}

void VulkanRenderer::Initialize()
{
    m_DeferredRenderer->Initialize();
}

void VulkanRenderer::Render()
{
    VkCommandBuffer cmd = m_SwapchainRenderer->BeginFrame(m_FrameIndex);

    if (cmd != VK_NULL_HANDLE)
    {
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(cmd, &beginInfo);

        m_DeferredRenderer->Render(cmd, m_FrameIndex, m_SwapchainRenderer->CurrentImageIndex());

        vkEndCommandBuffer(cmd);

        m_SwapchainRenderer->EndFrame(m_FrameIndex);
    }

    m_FrameIndex = (m_FrameIndex + 1) % VulkanSwapchain::MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::OnSwapchainRecreate(uint32_t width, uint32_t height)
{
    m_DeferredRenderer->Resize(width, height);
}

void VulkanRenderer::Shutdown()
{

}

