#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <vulkan/vulkan.h>
#include "vulkan_context.h"
#include "vulkan_image_utils.h"

struct ImageSpecification
{
    std::string DebugName;
    ImageFormat Format = ImageFormat::RGBA;
    ImageUsage Usage = ImageUsage::Texture;
    VkMemoryPropertyFlags Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bool UsedInTransferOps = false;
    uint32_t Width = 1;
    uint32_t Height = 1;
    uint32_t Mips = 1;
    uint32_t Layers = 1;
    bool CreateSampler = true;
    VkImage ExistingImage = VK_NULL_HANDLE;  // For swapchain images
    VkFormat SwapchainFormat = VK_FORMAT_UNDEFINED;
};

class VulkanImageView;

class VulkanImage2D
{
public:
    explicit VulkanImage2D(ImageSpecification specification);
    ~VulkanImage2D();

    VulkanImage2D(const VulkanImage2D&) = delete;
    VulkanImage2D& operator=(const VulkanImage2D&) = delete;

    void Release();
	void ReleaseSwapchainResources();
    void Invalidate();
    void Resize(uint32_t width, uint32_t height);

    VulkanImageView* GetView(uint32_t mip = 0);
    void TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = VK_REMAINING_MIP_LEVELS);
    void TransitionLayout(VkImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = VK_REMAINING_MIP_LEVELS);
    const VkDescriptorImageInfo& GetDescriptorInfo(uint32_t mip = 0) const;
    void CopyFromBufferAndGenerateMipmaps(VkBuffer buffer, VkDeviceSize bufferSize, uint32_t mipLevels);

    const ImageSpecification& GetSpecification() const { return m_Specification; }
    VkImage GetVkImage() const { return m_Image; }
    VkSampler GetSampler() const { return m_Sampler; }
    VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }
    void SetExpectedLayout(VkImageLayout expectedLayout);

private:
    void HandleSwapchainImage();
    void CreateImage();
    void CreateImageView(uint32_t mip);
    void UpdateImageViews();
    void CreateSampler();
    void GenerateMipmaps(VkCommandBuffer commandBuffer, uint32_t mipLevels);

    VkImageUsageFlags DetermineImageUsageFlags() const;
    void SetupImageSharingMode(VkImageCreateInfo& imageCreateInfo);
    VkImageLayout DetermineInitialLayout() const;
    void DetermineStageFlags(VkImageLayout oldLayout, VkImageLayout newLayout,
                             VkPipelineStageFlags& srcStage, VkPipelineStageFlags& dstStage,
                             VkAccessFlags& srcAccess, VkAccessFlags& dstAccess);

    static void CreateVkImageWithInfo(const VkImageCreateInfo& imageInfo,
                                      VkMemoryPropertyFlags properties,
                                      VkImage& image,
                                      VkDeviceMemory& imageMemory);

    ImageSpecification m_Specification;
    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
    VkSampler m_Sampler = VK_NULL_HANDLE;
    VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    std::map<uint32_t, std::unique_ptr<VulkanImageView>> m_MipViews;
    std::vector<VkImageLayout> m_MipLayouts;
    std::vector<uint32_t> m_ConcurrentQueueIndices;
};

class VulkanImageView
{
public:
    VulkanImageView(VulkanImage2D* image, uint32_t mip);
    ~VulkanImageView();

    VulkanImageView(const VulkanImageView&) = delete;
    VulkanImageView& operator=(const VulkanImageView&) = delete;

    VkImageView GetImageView() const { return m_ImageView; }
    void SetImageView(VkImageView view) { m_ImageView = view; }
    const VkDescriptorImageInfo& GetDescriptorInfo() const { return m_DescriptorImageInfo; }

    void UpdateDescriptorInfo(VkImageLayout layout, VkSampler sampler);

private:
    VulkanImage2D* m_Image;
    uint32_t m_Mip;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkDescriptorImageInfo m_DescriptorImageInfo{};
};