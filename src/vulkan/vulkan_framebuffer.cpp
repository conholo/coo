#include "vulkan_framebuffer.h"
#include "vulkan_context.h"
#include "vulkan_utils.h"

VulkanFramebuffer::VulkanFramebuffer(std::string debugName)
        : m_DebugName(std::move(debugName)), m_Framebuffer(VK_NULL_HANDLE), m_Width(0), m_Height(0), m_Layers(0)
{
}

VulkanFramebuffer::~VulkanFramebuffer()
{
    Destroy();
}

void VulkanFramebuffer::Create(
        VkRenderPass renderPass,
        const std::vector<VkImageView> &attachments,
        uint32_t width, uint32_t height,
        uint32_t layers)
{
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = layers;

    VK_CHECK_RESULT(vkCreateFramebuffer(VulkanContext::Get().Device(), &framebufferInfo, nullptr, &m_Framebuffer));

    m_Width = width;
    m_Height = height;
    m_Layers = layers;
}

void VulkanFramebuffer::Destroy()
{
    if (m_Framebuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(VulkanContext::Get().Device(), m_Framebuffer, nullptr);
        m_Framebuffer = VK_NULL_HANDLE;
    }
}