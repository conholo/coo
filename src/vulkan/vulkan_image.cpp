#include "vulkan_image.h"

#include "vulkan_context.h"
#include "vulkan_sampler_builder.h"
#include "vulkan_utils.h"

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
	if(m_Specification.Usage != ImageUsage::Swapchain)
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
			m_Image = VK_NULL_HANDLE;
        }
    }

    if (m_DeviceMemory != VK_NULL_HANDLE)
    {
        if(m_Specification.Usage != ImageUsage::Swapchain)
        {
            vkFreeMemory(VulkanContext::Get().Device(), m_DeviceMemory, nullptr);
			m_DeviceMemory = VK_NULL_HANDLE;
        }
    }
}

void VulkanImage2D::ReleaseSwapchainResources()
{
	if(m_Specification.Usage != ImageUsage::Swapchain)
		return;

	// Invoke destructor of VulkanImageView to free the VkImageView via clear().
	m_MipViews.clear();

	// The image and device memory will be freed by the implementation.
	m_Image = VK_NULL_HANDLE;
	m_DeviceMemory = VK_NULL_HANDLE;
}

void VulkanImage2D::Invalidate()
{
    assert(m_Specification.Width > 0 && m_Specification.Height > 0);
    Release();
    m_MipLayouts.resize(m_Specification.Mips, VK_IMAGE_LAYOUT_UNDEFINED);

    if (m_Specification.Usage == ImageUsage::Swapchain)
    {
        HandleSwapchainImage();
        SetExpectedLayout(VK_IMAGE_LAYOUT_UNDEFINED);
    }
    else
    {
        CreateImage();
        if (m_Specification.CreateSampler)
        {
            CreateSampler();
        }

        VkImageLayout initialLayout = DetermineInitialLayout();
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
	VulkanSamplerBuilder builder;

	builder.SetAnisotropy(16.0f)
		.SetForIntegerFormat(ImageUtils::IsIntegerBased(m_Specification.Format))
		.SetFilter(m_Specification.SamplerSpec.MinFilter, m_Specification.SamplerSpec.MagFilter)
		.SetMipmapMode(m_Specification.SamplerSpec.MipMapMode)
		.SetAddressMode(m_Specification.SamplerSpec.AddressModeU, m_Specification.SamplerSpec.AddressModeV, m_Specification.SamplerSpec.AddressModeW);
	
	m_Sampler = builder.Build();
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
        return it->second->GetDescriptorInfo();
    }

    // If the requested mip level doesn't exist, return the base mip level
    return m_MipViews.at(0)->GetDescriptorInfo();
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
        GenerateMipmaps(commandBuffer, mipLevels);
    }
    else
    {
        TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
    }

    VulkanContext::Get().EndSingleTimeCommand(commandBuffer);
}

void VulkanImage2D::GenerateMipmaps(VkCommandBuffer commandBuffer, uint32_t mipLevels)
{
    TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 1);

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, i, 1);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {int32_t(m_Specification.Width >> (i - 1)), int32_t(m_Specification.Height >> (i - 1)), 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {int32_t(m_Specification.Width >> i), int32_t(m_Specification.Height >> i), 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
                       m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit,
                       VK_FILTER_LINEAR);

        TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, i, 1);
    }

    TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, mipLevels);
}


void VulkanImage2D::UpdateImageViews()
{
    for (auto &[mip, view]: m_MipViews)
        view->UpdateDescriptorInfo(m_CurrentLayout, m_Sampler);

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
    barrier.subresourceRange.levelCount = levelCount == VK_REMAINING_MIP_LEVELS ? m_Specification.Mips - baseMipLevel : levelCount;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = m_Specification.Layers;

    VkPipelineStageFlags srcStage, dstStage;
    DetermineStageFlags(barrier.oldLayout, newLayout, srcStage, dstStage, barrier.srcAccessMask, barrier.dstAccessMask);

    vkCmdPipelineBarrier(
            commandBuffer,
            srcStage, dstStage,
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
        SetExpectedLayout(newLayout);
    }
}

void VulkanImage2D::DetermineStageFlags(VkImageLayout oldLayout, VkImageLayout newLayout,
                                        VkPipelineStageFlags& srcStage, VkPipelineStageFlags& dstStage,
                                        VkAccessFlags& srcAccess, VkAccessFlags& dstAccess)
{
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        srcAccess = 0;
        dstAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
        dstAccess = VK_ACCESS_SHADER_READ_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        srcAccess = 0;
        dstAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        srcAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dstAccess = VK_ACCESS_SHADER_READ_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        srcAccess = VK_ACCESS_SHADER_READ_BIT;
        dstAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    else
    {
        srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        srcAccess = 0;
        dstAccess = 0;
    }
}

VkImageLayout VulkanImage2D::DetermineInitialLayout() const
{
    switch (m_Specification.Usage)
    {
        case ImageUsage::Storage:
            return VK_IMAGE_LAYOUT_GENERAL;
        case ImageUsage::Attachment:
            return ImageUtils::IsDepthFormat(m_Specification.Format)
                   ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                   : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ImageUsage::Texture:
            return m_Specification.Mips > 1
                   ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                   : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case ImageUsage::HostRead:
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        default:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
}

void VulkanImage2D::SetExpectedLayout(VkImageLayout expectedLayout)
{
    m_CurrentLayout = expectedLayout;
    std::fill(m_MipLayouts.begin(), m_MipLayouts.end(), expectedLayout);

    for (auto& [mip, view] : m_MipViews)
    {
        view->UpdateDescriptorInfo(expectedLayout, m_Sampler);
    }
}

void VulkanImage2D::TransitionLayout(VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount)
{
	auto cmd = VulkanContext::Get().BeginSingleTimeCommands();
	TransitionLayout(cmd, newLayout, baseMipLevel, levelCount);
	VulkanContext::Get().EndSingleTimeCommand(cmd);
}

VulkanImageView::VulkanImageView(VulkanImage2D* image, uint32_t mip)
        : m_Image(image), m_Mip(mip)
{
}

VulkanImageView::~VulkanImageView()
{
    if (m_ImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(VulkanContext::Get().Device(), m_ImageView, nullptr);
        m_ImageView = VK_NULL_HANDLE;
    }
}

void VulkanImageView::UpdateDescriptorInfo(VkImageLayout layout, VkSampler sampler)
{
    m_DescriptorImageInfo.imageLayout = layout;
    m_DescriptorImageInfo.imageView = m_ImageView;
    m_DescriptorImageInfo.sampler = sampler;
}