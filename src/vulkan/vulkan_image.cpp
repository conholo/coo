#include "vulkan_image.h"
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
    if(m_Specification.InvalidateOnConstruction)
        Invalidate();
}

VulkanImage2D::~VulkanImage2D()
{
    Release();
}

void VulkanImage2D::Release()
{
    m_MipViews.clear();  // Destroys all image views

    if (m_Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(VulkanContext::Get().Device(), m_Sampler, nullptr);
        m_Sampler = VK_NULL_HANDLE;
    }

    if (m_Image != VK_NULL_HANDLE)
    {
        vkDestroyImage(VulkanContext::Get().Device(), m_Image, nullptr);
        m_Image = VK_NULL_HANDLE;
    }

    if (m_DeviceMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(VulkanContext::Get().Device(), m_DeviceMemory, nullptr);
        m_DeviceMemory = VK_NULL_HANDLE;
    }
}

void VulkanImage2D::Invalidate()
{
    assert(m_Specification.Width > 0 && m_Specification.Height > 0);
    Release();

    if (m_Specification.Usage == ImageUsage::Swapchain)
    {
        HandleSwapchainImage();
    }
    else
    {
        CreateImage();
        if (m_Specification.CreateSampler)
        {
            CreateSampler();
        }
        TransitionImageLayout();
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

void VulkanImage2D::TransitionImageLayout()
{
    if (m_Specification.Usage == ImageUsage::Storage || m_Specification.Usage == ImageUsage::HostRead)
    {
        VkCommandBuffer commandBuffer = VulkanContext::Get().BeginSingleTimeCommands();

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = m_Specification.Mips;
        subresourceRange.layerCount = m_Specification.Layers;

        VkImageLayout newLayout = (m_Specification.Usage == ImageUsage::Storage) ? VK_IMAGE_LAYOUT_GENERAL
                                                                                 : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        ImageUtils::InsertImageMemoryBarrier(
                commandBuffer,
                m_Image,
                0, 0,
                VK_IMAGE_LAYOUT_UNDEFINED,
                newLayout,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                subresourceRange);

        VulkanContext::Get().EndSingleTimeCommand(commandBuffer);
    }
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
        return it->second->GetDescriptorInfo();

    // If the requested mip level doesn't exist, return the base mip level
    return m_MipViews.at(0)->GetDescriptorInfo();
}

void VulkanImage2D::Resize(uint32_t width, uint32_t height)
{
    m_Specification.Width = width;
    m_Specification.Height = height;
    Invalidate();
}

void VulkanImage2D::UpdateImageViews()
{
    VkImageLayout layout;

    // Determine the appropriate image layout based on the image specification
    if (m_Specification.Usage == ImageUsage::Swapchain)
    {
        layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    else if (m_Specification.Usage == ImageUsage::Storage)
    {
        layout = VK_IMAGE_LAYOUT_GENERAL;
    }
    else if (ImageUtils::IsDepthFormat(m_Specification.Format))
    {
        if (m_Specification.Usage == ImageUsage::Attachment)
        {
            layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        else
        {
            layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        }
    }
    else if (m_Specification.Usage == ImageUsage::Attachment)
    {
        layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    else if (m_Specification.Usage == ImageUsage::Texture)
    {
        layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    else if (m_Specification.Usage == ImageUsage::HostRead)
    {
        layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    }
    else
    {
        // Default case
        layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // Update descriptor info for all existing views
    for (auto &[mip, view]: m_MipViews)
        view->UpdateDescriptorInfo(layout, m_Sampler);

    // If we have no views (which shouldn't happen, but just in case), create the base view
    if (m_MipViews.empty())
        CreateImageView(0);
}