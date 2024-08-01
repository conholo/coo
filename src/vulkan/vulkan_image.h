#pragma once

#include <string>
#include <memory>
#include "core/buffer.h"
#include "vulkan_context.h"
#include "vulkan_image_utils.h"

#include <vulkan/vulkan.h>
#include <map>

struct ImageSpecification
{
    std::string DebugName;

    ImageFormat Format = ImageFormat::RGBA;
    ImageUsage Usage = ImageUsage::Texture;
    VkMemoryPropertyFlags Properties;
    bool UsedInTransferOps = false;
    uint32_t Width = 1;
    uint32_t Height = 1;
    uint32_t Mips = 1;
    uint32_t Layers = 1;
    bool CreateSampler = true;
    VkImage ExistingImage = VK_NULL_HANDLE;  // For swapchain images
    VkFormat SwapchainFormat = VK_FORMAT_UNDEFINED;
    bool InvalidateOnConstruction = false;
};

class VulkanImageView;

class VulkanImage2D
{
public:
    VulkanImage2D(ImageSpecification specification);
    ~VulkanImage2D();

    void Release();
    void Invalidate();
    void Resize(uint32_t width, uint32_t height);

    VulkanImageView* GetView(uint32_t mip = 0);

    const VkDescriptorImageInfo &GetDescriptorInfo(uint32_t mip = 0) const;
    const ImageSpecification &GetSpecification() const { return m_Specification; }
    bool IsSharedConcurrently() const { return m_ConcurrentQueueIndices.size() < 2;}
    VkImage GetVkImage() const { return m_Image; }
    VkSampler GetSampler() const { return m_Sampler; }

private:
    void HandleSwapchainImage();
    void CreateImage();
    void CreateImageView(uint32_t mip);
    void UpdateImageViews();
    void CreateSampler();
    void TransitionImageLayout();

    VkImageUsageFlags DetermineImageUsageFlags() const;
    void SetupImageSharingMode(VkImageCreateInfo& imageCreateInfo);

    ImageSpecification m_Specification;
    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
    VkSampler m_Sampler = VK_NULL_HANDLE;
    std::map<uint32_t, std::unique_ptr<VulkanImageView>> m_MipViews;

    std::vector<uint32_t> m_ConcurrentQueueIndices;
    static void CreateVkImageWithInfo(const VkImageCreateInfo &imageInfo,
                                      VkMemoryPropertyFlags properties,
                                      VkImage &image,
                                      VkDeviceMemory &imageMemory);
};
class VulkanImageView
{
public:
    VulkanImageView(VulkanImage2D *image, uint32_t mip)
        : m_Image(image), m_Mip(mip)
    {
    }

    ~VulkanImageView()
    {
        if (m_ImageView != VK_NULL_HANDLE)
            vkDestroyImageView(VulkanContext::Get().Device(), m_ImageView, nullptr);
    }

    VkImageView ImageView() const { return m_ImageView; }
    void SetImageView(VkImageView view) { m_ImageView = view; }
    const VkDescriptorImageInfo& GetDescriptorInfo() const { return m_DescriptorImageInfo; }

    void UpdateDescriptorInfo(VkImageLayout layout, VkSampler sampler)
    {
        m_DescriptorImageInfo.imageLayout = layout;
        m_DescriptorImageInfo.imageView = m_ImageView;
        m_DescriptorImageInfo.sampler = sampler;
    }
private:
    VulkanImage2D *m_Image;
    uint32_t m_Mip;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkDescriptorImageInfo m_DescriptorImageInfo{};
};