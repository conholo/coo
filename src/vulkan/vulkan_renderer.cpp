#include "vulkan_renderer.h"
#include "core/frame_info.h"
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
    for(auto& uboBuffer : m_GlobalUboBuffers)
    {
        uboBuffer = std::make_shared<VulkanBuffer>(
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        uboBuffer->Map();
    }

    m_DeferredRenderer->Initialize();
}

void VulkanRenderer::Render(FrameInfo& frameInfo)
{
    GlobalUbo ubo
    {
        .Projection = frameInfo.Cam.GetProjection(),
        .View = frameInfo.Cam.GetView(),
        .InvView = frameInfo.Cam.GetInvView(),
        .InvProjection = frameInfo.Cam.GetInvProjection(),
        .CameraPosition = glm::vec4(frameInfo.Cam.GetPosition(), 1.0)
    };
    frameInfo.GlobalUbo = m_GlobalUboBuffers[frameInfo.FrameIndex];
    frameInfo.GlobalUbo->WriteToBuffer(&ubo);
    frameInfo.GlobalUbo->Flush();

    VkCommandBuffer cmd = m_SwapchainRenderer->BeginFrame(frameInfo.FrameIndex);
    frameInfo.CommandBuffer = cmd;

    if (cmd != VK_NULL_HANDLE)
    {
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(cmd, &beginInfo);

        m_DeferredRenderer->Render(frameInfo);

        vkEndCommandBuffer(cmd);

        m_SwapchainRenderer->EndFrame(frameInfo.FrameIndex);
    }
}

void VulkanRenderer::OnSwapchainRecreate(uint32_t width, uint32_t height)
{
    m_DeferredRenderer->Resize(width, height);
}

void VulkanRenderer::Shutdown()
{

}
