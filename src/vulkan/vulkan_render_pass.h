#pragma once

#include <vulkan/vulkan.h>

#include "vulkan_image_utils.h"
#include "vulkan_swapchain.h"
#include <vector>
#include <optional>
#include <string>

enum class AttachmentType
{
    Color,
    Depth,
    DepthStencil,
    Resolve
};

struct AttachmentDescription
{
    AttachmentType Type{};
    ImageFormat Format{};
    VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;
    VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkAttachmentLoadOp StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkImageLayout InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkClearValue ClearValue{};
};

struct SubpassDescription
{
    std::vector <uint32_t> ColorAttachments;
    std::optional <uint32_t> DepthStencilAttachment;
    std::vector <uint32_t> InputAttachments;
    std::vector <uint32_t> ResolveAttachments;
    std::vector <uint32_t> PreserveAttachments;
};

class VulkanRenderPass
{
public:
    explicit VulkanRenderPass(std::string debugName);
    ~VulkanRenderPass();

    void AddAttachment(const AttachmentDescription &attachment);
    void AddSubpass(const SubpassDescription &subpass);
    void AddDependency(
            uint32_t srcSubpass, uint32_t dstSubpass,
            VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            VkDependencyFlags dependencyFlags);
    void Build();

    void BeginPass(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo beginInfo, VkExtent2D extent);
    void EndPass(VkCommandBuffer commandBuffer);

    VkRenderPass RenderPass() const { return m_RenderPass; }
    uint32_t ColorAttachmentCount();
    static bool FormatIsDepth(ImageFormat format);

    const std::vector<AttachmentDescription>& GetAttachmentDescriptions() const { return m_Attachments; }

private:
    std::string m_DebugName;
    std::vector <AttachmentDescription> m_Attachments;
    std::vector <SubpassDescription> m_Subpasses;
    std::vector <VkSubpassDependency> m_Dependencies;
    std::vector <VkClearValue> m_AttachmentClearValues;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;

    void CreateRenderPass();
};