#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

class VulkanFramebuffer
{
public:
    explicit VulkanFramebuffer(std::string debugName = "Framebuffer");
    ~VulkanFramebuffer();

    void Create(VkRenderPass renderPass,
                const std::vector<VkImageView> &attachments,
                uint32_t width, uint32_t height,
                uint32_t layers = 1);

    void Destroy();

    VkFramebuffer Framebuffer() const { return m_Framebuffer; }
    uint32_t Width() const { return m_Width; }
    uint32_t Height() const { return m_Height; }
    uint32_t LayerCount() const { return m_Layers; }

private:
    std::string m_DebugName;
    VkFramebuffer m_Framebuffer;
    uint32_t m_Width;
    uint32_t m_Height;
    uint32_t m_Layers;
};