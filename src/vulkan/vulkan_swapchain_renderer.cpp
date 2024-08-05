#include "vulkan_swapchain_renderer.h"
#include <array>
#include <stdexcept>


VulkanSwapchainRenderer::VulkanSwapchainRenderer(Window &windowRef)
        : m_WindowRef(windowRef)
{
    RecreateSwapchain();
    CreateDrawCommandBuffers();
    CreateSyncObjects();
}

VulkanSwapchainRenderer::~VulkanSwapchainRenderer()
{
    FreeCommandBuffers();
}

void VulkanSwapchainRenderer::RecreateSwapchain()
{
    auto extent = m_WindowRef.GetExtent();
    while (extent.width == 0 || extent.height == 0)
    {
        extent = m_WindowRef.GetExtent();
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(VulkanContext::Get().Device());

    if (m_Swapchain == nullptr)
    {
        m_Swapchain = std::make_unique<VulkanSwapchain>(extent);
    }
    else
    {
        std::shared_ptr<VulkanSwapchain> oldSwapChain = std::move(m_Swapchain);
        m_Swapchain = std::make_unique<VulkanSwapchain>(extent, oldSwapChain);

        if (!oldSwapChain->CompareFormats(*m_Swapchain))
            throw std::runtime_error("Swap chain imageInfo format has changed!");
    }

    if (m_RecreateSwapchainCallback)
        m_RecreateSwapchainCallback(m_Swapchain->Width(), m_Swapchain->Height());
}

void VulkanSwapchainRenderer::CreateDrawCommandBuffers()
{
    m_DrawCommandBuffers.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = VulkanContext::Get().GraphicsCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_DrawCommandBuffers.size());

    if (vkAllocateCommandBuffers(VulkanContext::Get().Device(), &allocInfo, m_DrawCommandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void VulkanSwapchainRenderer::FreeCommandBuffers()
{
    vkFreeCommandBuffers(
            VulkanContext::Get().Device(),
            VulkanContext::Get().GraphicsCommandPool(),
            static_cast<uint32_t>(m_DrawCommandBuffers.size()),
            m_DrawCommandBuffers.data());
    m_DrawCommandBuffers.clear();
}

VkCommandBuffer VulkanSwapchainRenderer::BeginFrame(uint32_t frameIndex)
{
    vkWaitForFences(
            VulkanContext::Get().Device(),
            1,
            &m_InFlightFences[frameIndex],
            VK_TRUE,
            std::numeric_limits<uint64_t>::max());

    auto result = m_Swapchain->AcquireNextImage(&m_CurrentImageIndex, m_PresentCompleteSemaphores[frameIndex]);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapchain();
        return VK_NULL_HANDLE;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to acquire swap chain imageInfo!");

    return m_DrawCommandBuffers[frameIndex];
}

void VulkanSwapchainRenderer::EndFrame(uint32_t frameIndex)
{
    if (m_ImagesInFlight[m_CurrentImageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(VulkanContext::Get().Device(),
                        1,
                        &m_ImagesInFlight[m_CurrentImageIndex],
                        VK_TRUE,
                        std::numeric_limits<uint64_t>::max());
    }
    m_ImagesInFlight[m_CurrentImageIndex] = m_InFlightFences[frameIndex];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    std::vector<VkSemaphore> waitSemaphores{m_PresentCompleteSemaphores[frameIndex]};
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();

    std::vector<VkPipelineStageFlags> waitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages.data();

    std::vector<VkSemaphore> signalSemaphores{m_RenderFinishedSemaphores[frameIndex]};
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_DrawCommandBuffers[frameIndex];

    vkResetFences(VulkanContext::Get().Device(), 1, &m_InFlightFences[frameIndex]);

    if (vkQueueSubmit(VulkanContext::Get().GraphicsQueue(), 1, &submitInfo, m_InFlightFences[frameIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[frameIndex];

    VkSwapchainKHR swapchains[] = {m_Swapchain->Swapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &m_CurrentImageIndex;

    auto result = vkQueuePresentKHR(VulkanContext::Get().PresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_WindowRef.WasWindowResized())
    {
        m_WindowRef.ResetWindowResizedFlag();
        RecreateSwapchain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present swapchain imageInfo!");
    }
}

void VulkanSwapchainRenderer::CreateSyncObjects()
{
    auto device = VulkanContext::Get().Device();
    m_PresentCompleteSemaphores.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
    m_ImagesInFlight.resize(m_Swapchain->ImageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_PresentCompleteSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
}
