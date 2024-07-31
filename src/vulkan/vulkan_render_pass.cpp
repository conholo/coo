#include "vulkan_render_pass.h"

#include <utility>
#include "vulkan_context.h"
#include "vulkan_utils.h"

VulkanRenderPass::VulkanRenderPass(std::string debugName)
    : m_DebugName(std::move(debugName))
{
}

VulkanRenderPass::~VulkanRenderPass()
{
    if (m_RenderPass != VK_NULL_HANDLE)
        vkDestroyRenderPass(VulkanContext::Get().Device(), m_RenderPass, nullptr);
}

void VulkanRenderPass::AddAttachment(const AttachmentDescription &attachment)
{
    m_Attachments.push_back(attachment);
}

void VulkanRenderPass::AddSubpass(const SubpassDescription &subpass)
{
    m_Subpasses.push_back(subpass);
}

void VulkanRenderPass::AddDependency(uint32_t srcSubpass, uint32_t dstSubpass, VkPipelineStageFlags srcStageMask,
                               VkPipelineStageFlags dstStageMask, VkAccessFlags srcAccessMask,
                               VkAccessFlags dstAccessMask, VkDependencyFlags dependencyFlags)
{
    VkSubpassDependency dependency{};
    dependency.srcSubpass = srcSubpass;
    dependency.dstSubpass = dstSubpass;
    dependency.srcStageMask = srcStageMask;
    dependency.dstStageMask = dstStageMask;
    dependency.srcAccessMask = srcAccessMask;
    dependency.dstAccessMask = dstAccessMask;
    dependency.dependencyFlags = dependencyFlags;
    m_Dependencies.push_back(dependency);
}

void VulkanRenderPass::Build()
{
    CreateRenderPass();
}

void VulkanRenderPass::CreateRenderPass()
{
    std::vector <VkAttachmentDescription> attachmentDescriptions;
    for (const auto &attachment: m_Attachments)
    {
        VkAttachmentDescription desc{};
        desc.format = ImageUtils::VulkanImageFormat(attachment.Format);
        desc.samples = attachment.Samples;
        desc.loadOp = attachment.LoadOp;
        desc.storeOp = attachment.StoreOp;
        desc.stencilLoadOp = attachment.StencilLoadOp;
        desc.stencilStoreOp = attachment.StencilStoreOp;
        desc.initialLayout = attachment.InitialLayout;
        desc.finalLayout = attachment.FinalLayout;
        attachmentDescriptions.push_back(desc);
    }

    std::vector <VkSubpassDescription> subpassDescriptions;
    std::vector <std::vector<VkAttachmentReference>> colorReferences;
    std::vector <VkAttachmentReference> depthReferences;
    std::vector <std::vector<VkAttachmentReference>> inputReferences;
    std::vector <std::vector<VkAttachmentReference>> resolveReferences;

    for (const auto &subpass: m_Subpasses)
    {
        VkSubpassDescription desc{};
        desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        colorReferences.emplace_back();
        for (uint32_t attachment: subpass.ColorAttachments)
        {
            colorReferences.back().push_back({attachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        }
        desc.colorAttachmentCount = static_cast<uint32_t>(colorReferences.back().size());
        desc.pColorAttachments = colorReferences.back().data();

        if (subpass.DepthStencilAttachment)
        {
            depthReferences.push_back({subpass.DepthStencilAttachment.value(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});
            desc.pDepthStencilAttachment = &depthReferences.back();
        }

        inputReferences.emplace_back();
        for (uint32_t attachment: subpass.InputAttachments)
        {
            inputReferences.back().push_back({attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        }
        desc.inputAttachmentCount = static_cast<uint32_t>(inputReferences.back().size());
        desc.pInputAttachments = inputReferences.back().data();

        resolveReferences.emplace_back();
        for (uint32_t attachment: subpass.ResolveAttachments)
        {
            resolveReferences.back().push_back({attachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        }
        desc.pResolveAttachments = resolveReferences.back().data();

        desc.preserveAttachmentCount = static_cast<uint32_t>(subpass.PreserveAttachments.size());
        desc.pPreserveAttachments = subpass.PreserveAttachments.data();

        subpassDescriptions.push_back(desc);
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
    renderPassInfo.pSubpasses = subpassDescriptions.data();
    renderPassInfo.dependencyCount = static_cast<uint32_t>(m_Dependencies.size());
    renderPassInfo.pDependencies = m_Dependencies.data();

    VK_CHECK_RESULT(vkCreateRenderPass(VulkanContext::Get().Device(), &renderPassInfo, nullptr, &m_RenderPass));
}