#include "vulkan_texture.h"
#include "vulkan_context.h"
#include "vulkan_buffer.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <utility>

std::shared_ptr<VulkanTexture2D> VulkanTexture2D::CreateFromFile(const TextureSpecification& specification, const std::string& filepath)
{
    auto texture = std::make_shared<VulkanTexture2D>(specification);
    texture->LoadFromFile(filepath);
    texture->Invalidate(false);
    return texture;
}

std::shared_ptr<VulkanTexture2D> VulkanTexture2D::CreateFromMemory(const TextureSpecification& specification, const Buffer& data)
{
    auto texture = std::make_shared<VulkanTexture2D>(specification);
    texture->LoadFromMemory(data);
    texture->Invalidate(false);
    return texture;
}

std::shared_ptr<VulkanTexture2D> VulkanTexture2D::CreateAttachment(const TextureSpecification& specification)
{
    auto spec = specification;
    spec.Usage = TextureUsage::Attachment;
    auto texture = std::make_shared<VulkanTexture2D>(spec);
    texture->Invalidate();
    return texture;
}

VulkanTexture2D::VulkanTexture2D(TextureSpecification specification)
        : m_Specification(std::move(specification))
{
}

VulkanTexture2D::~VulkanTexture2D()
{
    Release();
}

VulkanTexture2D::VulkanTexture2D(VulkanTexture2D&& other) noexcept
        : m_Specification(std::move(other.m_Specification)),
          m_Filepath(std::move(other.m_Filepath)),
          m_ImageData(std::move(other.m_ImageData)),
          m_Image(std::move(other.m_Image)),
          m_DescriptorInfo(other.m_DescriptorInfo)
{
    other.m_DescriptorInfo = {};
}

VulkanTexture2D& VulkanTexture2D::operator=(VulkanTexture2D&& other) noexcept
{
    if (this != &other)
    {
        Release();
        m_Specification = std::move(other.m_Specification);
        m_Filepath = std::move(other.m_Filepath);
        m_ImageData = std::move(other.m_ImageData);
        m_Image = std::move(other.m_Image);
        m_DescriptorInfo = other.m_DescriptorInfo;
        other.m_DescriptorInfo = {};
    }
    return *this;
}

void VulkanTexture2D::Release()
{
    m_Image.reset();
    m_ImageData.Release();
    m_DescriptorInfo = {};
}

void VulkanTexture2D::Invalidate(bool release)
{
    if(release)
        Release();

    if (m_Specification.Usage == TextureUsage::Attachment)
    {
        CreateAttachmentImage();
    }
    else
    {
        CreateTextureImage();
    }

    UpdateDescriptorInfo();
}

void VulkanTexture2D::Resize(uint32_t width, uint32_t height)
{
    if (m_Specification.Width == width && m_Specification.Height == height)
        return;

    m_Specification.Width = width;
    m_Specification.Height = height;
    Invalidate();
}

void VulkanTexture2D::LoadFromFile(const std::string& filepath)
{
    m_Filepath = filepath;
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    void* data = nullptr;

    if (stbi_is_hdr(filepath.c_str()))
    {
        data = stbi_loadf(filepath.c_str(), &width, &height, &channels, 4);
        m_Specification.Format = ImageFormat::RGBA32F;
    }
    else
    {
        data = stbi_load(filepath.c_str(), &width, &height, &channels, 4);
        m_Specification.Format = ImageFormat::RGBA;
    }

    if (!data)
        throw std::runtime_error("Failed to load image: " + filepath);

    m_Specification.Width = width;
    m_Specification.Height = height;

    m_ImageData = Buffer::Copy(data, width * height * 4 * (m_Specification.Format == ImageFormat::RGBA32F ? sizeof(float) : sizeof(uint8_t)));
    stbi_image_free(data);
}

void VulkanTexture2D::LoadFromMemory(const Buffer& data)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    void* imageData = nullptr;

    if (stbi_is_hdr_from_memory(static_cast<const stbi_uc*>(data.Data()), static_cast<int>(data.GetSize())))
    {
        imageData = stbi_loadf_from_memory(static_cast<const stbi_uc*>(data.Data()), static_cast<int>(data.GetSize()), &width, &height, &channels, 4);
        m_Specification.Format = ImageFormat::RGBA32F;
    }
    else
    {
        imageData = stbi_load_from_memory(static_cast<const stbi_uc*>(data.Data()), static_cast<int>(data.GetSize()), &width, &height, &channels, 4);
        m_Specification.Format = ImageFormat::RGBA;
    }

    if (!imageData)
        throw std::runtime_error("Failed to load image from memory");

    m_Specification.Width = width;
    m_Specification.Height = height;

    m_ImageData = Buffer::Copy(imageData, width * height * 4 * (m_Specification.Format == ImageFormat::RGBA32F ? sizeof(float) : sizeof(uint8_t)));
    stbi_image_free(imageData);
}

void VulkanTexture2D::CreateTextureImage()
{
    VkDeviceSize imageSize = m_ImageData.GetSize();
    uint32_t mipLevels = m_Specification.GenerateMips ? ImageUtils::CalculateMipCount(m_Specification.Width, m_Specification.Height) : 1;

    VulkanBuffer stagingBuffer(
            imageSize,
            1,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.Map();
    stagingBuffer.WriteToBuffer(m_ImageData.Data(), imageSize);

    ImageSpecification imageSpec;
    imageSpec.Format = m_Specification.Format;
    imageSpec.Width = m_Specification.Width;
    imageSpec.Height = m_Specification.Height;
    imageSpec.Usage = ImageUsage::Texture;
    imageSpec.Mips = mipLevels;
    imageSpec.DebugName = m_Specification.DebugName;

    m_Image = std::make_unique<VulkanImage2D>(imageSpec);
    m_Image->CopyFromBufferAndGenerateMipmaps(stagingBuffer.GetBuffer(), imageSize, mipLevels);
}

void VulkanTexture2D::CreateAttachmentImage()
{
    ImageSpecification imageSpec;
    imageSpec.Format = m_Specification.Format;
    imageSpec.Width = m_Specification.Width;
    imageSpec.Height = m_Specification.Height;
    imageSpec.Usage = ImageUsage::Attachment;
    imageSpec.Mips = 1;
    imageSpec.DebugName = m_Specification.DebugName;
    imageSpec.Properties = m_Specification.MemoryProperties;
    imageSpec.UsedInTransferOps = m_Specification.UsedInTransferOps;

    m_Image = std::make_unique<VulkanImage2D>(imageSpec);
}

void VulkanTexture2D::UpdateDescriptorInfo()
{
    m_DescriptorInfo = m_Image->GetDescriptorInfo();
}

void VulkanTexture2D::TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout)
{
    m_Image->TransitionLayout(cmd, newLayout);
}