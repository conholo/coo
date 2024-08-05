#include "vulkan_image.h"
#include "vulkan_utils.h"
#include "vulkan_context.h"

#include <utility>

void VulkanImage2D::CreateVkImageWithInfo(const VkImageCreateInfo &imageInfo,
                                          VkMemoryPropertyFlags properties,
                                          VkImage &image,
                                          VkDeviceMemory &imageMemory)
{
    auto device = VulkanContext::Get().Device();
    VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = VulkanContext::Get().FindDeviceMemoryType(memRequirements.memoryTypeBits, properties);

    VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));
    VK_CHECK_RESULT(vkBindImageMemory(device, image, imageMemory, 0));
}

VulkanImage2D::VulkanImage2D(ImageSpecification specification)
    : m_Specification(std::move(specification))
{
    assert(m_Specification.Width > 0 && m_Specification.Height > 0);
    Invalidate();
}

VulkanImage2D::~VulkanImage2D()
{
    Release();
}

void VulkanImage2D::Release()
{
    m_MipViews.clear();  // Destroys all imageInfo views

    if (m_Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(VulkanContext::Get().Device(), m_Sampler, nullptr);
        m_Sampler = VK_NULL_HANDLE;
    }

    if (m_Image != VK_NULL_HANDLE)
    {
        if(m_Specification.Usage != ImageUsage::Swapchain)
        {
            vkDestroyImage(VulkanContext::Get().Device(), m_Image, nullptr);
        }
        m_Image = VK_NULL_HANDLE;
    }

    if (m_DeviceMemory != VK_NULL_HANDLE)
    {
        if(m_Specification.Usage != ImageUsage::Swapchain)
        {
            vkFreeMemory(VulkanContext::Get().Device(), m_DeviceMemory, nullptr);
        }
        m_DeviceMemory = VK_NULL_HANDLE;
    }
}

void VulkanImage2D::Invalidate()
{
    assert(m_Specification.Width > 0 && m_Specification.Height > 0);
    Release();
    m_MipLayouts.resize(m_Specification.Mips, VK_IMAGE_LAYOUT_UNDEFINED);

    if (m_Specification.Usage == ImageUsage::Swapchain)
    {
        HandleSwapchainImage();
        m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    else
    {
        CreateImage();
        if (m_Specification.CreateSampler)
        {
            CreateSampler();
        }

        VkImageLayout initialLayout;
        switch (m_Specification.Usage)
        {
            case ImageUsage::Storage:
                initialLayout = VK_IMAGE_LAYOUT_GENERAL;
                break;
            case ImageUsage::Attachment:
                initialLayout = ImageUtils::IsDepthFormat(m_Specification.Format)
                                ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                                : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                break;
            case ImageUsage::Texture:
                initialLayout = m_Specification.Mips > 1
                        ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                        : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                break;
            case ImageUsage::HostRead:
                initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                break;
            default:
                initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        VkCommandBuffer commandBuffer = VulkanContext::Get().BeginSingleTimeCommands();
        TransitionLayout(commandBuffer, initialLayout, 0, m_Specification.Mips);
        VulkanContext::Get().EndSingleTimeCommand(commandBuffer);
    }

    CreateImageView(0);
    UpdateImageViews();
}


void VulkanImage2D::HandleSwapchainImage()
{
    m_Image = m_Specification.ExistingImage;
    m_Sampler = VK_NULL_HANDLE;
}

void VulkanImage2D::CreateImage()
{
    VkImageUsageFlags usage = DetermineImageUsageFlags();
    VkFormat vulkanFormat = ImageUtils::VulkanImageFormat(m_Specification.Format);

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = vulkanFormat;
    imageCreateInfo.extent = {m_Specification.Width, m_Specification.Height, 1};
    imageCreateInfo.mipLevels = m_Specification.Mips;
    imageCreateInfo.arrayLayers = m_Specification.Layers;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = m_Specification.Usage == ImageUsage::HostRead
            ? VK_IMAGE_TILING_LINEAR
            : VK_IMAGE_TILING_OPTIMAL;

    imageCreateInfo.usage = usage;

    SetupImageSharingMode(imageCreateInfo);
    CreateVkImageWithInfo(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_DeviceMemory);
    SetDebugUtilsObjectName(VulkanContext::Get().Device(), VK_OBJECT_TYPE_IMAGE, (uint64_t)m_Image, m_Specification.DebugName.c_str());
}

VkImageUsageFlags VulkanImage2D::DetermineImageUsageFlags() const
{
    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    if (m_Specification.Usage == ImageUsage::Attachment)
    {
        usage |= ImageUtils::IsDepthFormat(m_Specification.Format)
                ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (m_Specification.UsedInTransferOps || m_Specification.Usage == ImageUsage::Texture)
    {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (m_Specification.Usage == ImageUsage::Storage)
    {
        usage |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    return usage;
}

void VulkanImage2D::SetupImageSharingMode(VkImageCreateInfo &imageCreateInfo)
{
    if (m_Specification.Usage == ImageUsage::Storage)
    {
        auto queueFamilyIndices = VulkanContext::Get().GetAvailableDeviceQueueFamilyIndices();

        // Check if graphics and compute queues are different
        if (queueFamilyIndices.GraphicsFamily.has_value() &&
            queueFamilyIndices.ComputeFamily.has_value() &&
            queueFamilyIndices.GraphicsFamily.value() != queueFamilyIndices.ComputeFamily.value())
        {
            m_ConcurrentQueueIndices =
            {
                queueFamilyIndices.GraphicsFamily.value(),
                queueFamilyIndices.ComputeFamily.value()
            };

            imageCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            imageCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(m_ConcurrentQueueIndices.size());
            imageCreateInfo.pQueueFamilyIndices = m_ConcurrentQueueIndices.data();
        }
        else
        {
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.queueFamilyIndexCount = 0;
            imageCreateInfo.pQueueFamilyIndices = nullptr;
        }
    }
    else
    {
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.queueFamilyIndexCount = 0;
        imageCreateInfo.pQueueFamilyIndices = nullptr;
    }
}

void VulkanImage2D::CreateSampler()
{
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.anisotropyEnable = VK_TRUE;
    samplerCreateInfo.maxAnisotropy = 16.0f;

    if (ImageUtils::IsIntegerBased(m_Specification.Format))
    {
        samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
        samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    } else
    {
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }

    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
    samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 100.0f;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    VK_CHECK_RESULT(vkCreateSampler(VulkanContext::Get().Device(), &samplerCreateInfo, nullptr, &m_Sampler));
}

VulkanImageView* VulkanImage2D::GetView(uint32_t mip)
{
    auto it = m_MipViews.find(mip);
    if (it == m_MipViews.end())
        CreateImageView(mip);

    return m_MipViews[mip].get();
}

void VulkanImage2D::CreateImageView(uint32_t mip)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_Image;
    viewInfo.viewType = m_Specification.Layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;

    if (m_Specification.Usage == ImageUsage::Swapchain)
    {
        viewInfo.format = m_Specification.SwapchainFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
    } else
    {
        viewInfo.format = ImageUtils::VulkanImageFormat(m_Specification.Format);
        viewInfo.subresourceRange.aspectMask = ImageUtils::IsDepthFormat(m_Specification.Format)
                                               ? VK_IMAGE_ASPECT_DEPTH_BIT
                                               : VK_IMAGE_ASPECT_COLOR_BIT;
        if (m_Specification.Format == ImageFormat::DEPTH24STENCIL8)
            viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        viewInfo.subresourceRange.levelCount = m_Specification.Mips - mip;
    }

    viewInfo.subresourceRange.baseMipLevel = mip;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = m_Specification.Layers;

    auto view = std::make_unique<VulkanImageView>(this, mip);
    VkImageView imageView;
    VK_CHECK_RESULT(vkCreateImageView(VulkanContext::Get().Device(), &viewInfo, nullptr, &imageView));
    view->SetImageView(imageView);
    m_MipViews[mip] = std::move(view);
}

const VkDescriptorImageInfo& VulkanImage2D::GetDescriptorInfo(uint32_t mip) const
{
    auto it = m_MipViews.find(mip);
    if (it != m_MipViews.end())
    {
        auto& info = it->second->GetDescriptorInfo();
        const_cast<VkDescriptorImageInfo&>(info).imageLayout = m_CurrentLayout;
        return info;
    }

    // If the requested mip level doesn't exist, return the base mip level
    auto& info = m_MipViews.at(0)->GetDescriptorInfo();
    const_cast<VkDescriptorImageInfo&>(info).imageLayout = m_CurrentLayout;
    return info;
}

void VulkanImage2D::Resize(uint32_t width, uint32_t height)
{
    m_Specification.Width = width;
    m_Specification.Height = height;
    Invalidate();
}

void VulkanImage2D::CopyFromBufferAndGenerateMipmaps(VkBuffer buffer, VkDeviceSize bufferSize, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = VulkanContext::Get().BeginSingleTimeCommands();

    TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 1);

    // Copy buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {m_Specification.Width, m_Specification.Height, 1};

    vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            m_Image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

    if (mipLevels > 1)
    {
        // Transition first mip level to transfer source for generating mipmaps
        TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 1);

        int32_t mipWidth = m_Specification.Width;
        int32_t mipHeight = m_Specification.Height;

        for (uint32_t i = 1; i < mipLevels; i++)
        {
            // Ensure the destination mip level is in the correct layout
            TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, i, 1);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                           m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit,
                           VK_FILTER_LINEAR);

            // Transition the current mip level to transfer source for the next iteration
            TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, i, 1);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        // After the loop, transition all mip levels to SHADER_READ_ONLY_OPTIMAL
        TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, mipLevels);
    }
    else
    {
        // If we're not generating mipmaps, transition the image to shader read optimal
        TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
    }

    VulkanContext::Get().EndSingleTimeCommand(commandBuffer);

    m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void VulkanImage2D::UpdateImageViews()
{
    // Update descriptor info for all existing views
    for (auto &[mip, view]: m_MipViews)
        view->UpdateDescriptorInfo(m_CurrentLayout, m_Sampler);

    // If we have no views (which shouldn't happen, but just in case), create the base view
    if (m_MipViews.empty())
        CreateImageView(0);
}

void VulkanImage2D::TransitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_MipLayouts[baseMipLevel];
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_Image;
    barrier.subresourceRange.aspectMask = ImageUtils::IsDepthFormat(m_Specification.Format)
                                          ? VK_IMAGE_ASPECT_DEPTH_BIT
                                          : VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = baseMipLevel;
    barrier.subresourceRange.levelCount = levelCount;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = m_Specification.Layers;

    ImageStage srcStage = ImageStage::TopOfPipe;
    ImageStage dstStage = ImageStage::AllCommands;

    if (barrier.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && barrier.newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = ImageStage::TopOfPipe;
        dstStage = ImageStage::Transfer;
    }
    else if (barrier.oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && barrier.newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = ImageStage::Transfer;
        dstStage = ImageStage::FragmentShader;
    }
    else if (barrier.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && barrier.newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        srcStage = ImageStage::TopOfPipe;
        dstStage = ImageStage::ColorAttachmentOutput;
    }

    VkPipelineStageFlags srcStageMask = GetVkPipelineStageFlags(srcStage);
    VkPipelineStageFlags dstStageMask = GetVkPipelineStageFlags(dstStage);

    vkCmdPipelineBarrier(
            commandBuffer,
            srcStageMask, dstStageMask,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
    );

    uint32_t affectedLevelCount = (levelCount == VK_REMAINING_MIP_LEVELS)
                                  ? (m_Specification.Mips - baseMipLevel)
                                  : levelCount;

    for (uint32_t i = 0; i < affectedLevelCount; ++i)
    {
        m_MipLayouts[baseMipLevel + i] = newLayout;
    }

    if (baseMipLevel == 0 && affectedLevelCount == m_Specification.Mips)
    {
        m_CurrentLayout = newLayout;
    }
}

VkPipelineStageFlags VulkanImage2D::GetVkPipelineStageFlags(ImageStage stage)
{
    switch (stage)
    {
        case ImageStage::TopOfPipe: return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case ImageStage::Transfer: return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case ImageStage::FragmentShader: return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case ImageStage::ColorAttachmentOutput: return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case ImageStage::EarlyFragmentTests: return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        case ImageStage::AllCommands: return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        default: return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }
}