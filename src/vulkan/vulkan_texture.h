#pragma once

#include <string>
#include <memory>
#include "vulkan_image.h"
#include "core/buffer.h"

enum class TextureUsage { Texture, Attachment, Storage };

struct TextureSpecification
{
    ImageFormat Format = ImageFormat::RGBA;
    TextureUsage Usage = TextureUsage::Texture;
    uint32_t Width = 1;
    uint32_t Height = 1;
    bool GenerateMips = true;
    VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    bool UsedInTransferOps = false;
	bool CreateSampler = true;
	SamplerSpecification SamplerSpec{};
    std::string DebugName;
};

class VulkanTexture2D
{
public:
    static std::unique_ptr<VulkanTexture2D> CreateFromFile(const TextureSpecification& specification, const std::string& filepath);
    static std::unique_ptr<VulkanTexture2D> CreateFromMemory(const TextureSpecification& specification, const Buffer& data);
    static std::unique_ptr<VulkanTexture2D> CreateAttachment(const TextureSpecification& specification);

    explicit VulkanTexture2D(TextureSpecification specification);
    ~VulkanTexture2D();

    VulkanTexture2D(const VulkanTexture2D&) = delete;
    VulkanTexture2D& operator=(const VulkanTexture2D&) = delete;
    VulkanTexture2D(VulkanTexture2D&&) noexcept;
    VulkanTexture2D& operator=(VulkanTexture2D&&) noexcept;

    void Release();
    void Invalidate(bool releaseExisting = true);
    void Resize(uint32_t width, uint32_t height);

    ImageFormat GetFormat() const { return m_Specification.Format; }
    uint32_t GetWidth() const { return m_Specification.Width; }
    uint32_t GetHeight() const { return m_Specification.Height; }
    const TextureSpecification& GetSpecification() const { return m_Specification; }
    VkImage GetVkImage() const;
    VkImageLayout GetCurrentLayout() const;
    bool HasMipmaps() const;

    VulkanImage2D* GetImage() const { return m_Image.get(); }
    VkDescriptorImageInfo GetBaseViewDescriptorInfo() const { return m_DescriptorInfo; }

    void UpdateState(VkImageLayout expectedLayout);
	void TransitionLayout(VkImageLayout newLayout);
    void TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout);

private:
    void LoadFromFile(const std::string& filepath);
    void LoadFromMemory(const Buffer& data);
    void CreateTextureImage();
    void CreateAttachmentImage();
    void CreateEmptyTextureImage();
    void UpdateDescriptorInfo();

    TextureSpecification m_Specification;
    std::string m_Filepath;
    Buffer m_ImageData;
    std::unique_ptr<VulkanImage2D> m_Image;
    VkDescriptorImageInfo m_DescriptorInfo{};
};
