#pragma once
#include <vulkan/vulkan.h>
#include <cassert>
#include <glm/gtc/integer.hpp>

enum class ImageFormat
{
    None = 0,
    RED8UN,
    RED8UI,
    RED16UI,
    RED32UI,
    RED32F,
    RG8,
    RG16F,
    RG32F,
    RGB,
    RGBA,
    RGBA16F,
    RGBA32F,

    B10R11G11UF,

    SRGB,

    DEPTH32FSTENCIL8UINT,
    DEPTH32F,
    DEPTH24STENCIL8,

    // Defaults
    Depth = DEPTH24STENCIL8,
};

enum class ImageUsage
{
    None = 0,
    Texture,
    Attachment,
    Storage,
    HostRead,
    Swapchain
};

enum class TextureWrap
{
    None = 0,
    Clamp,
    Repeat
};

enum class TextureFilter
{
    None = 0,
    Linear,
    Nearest,
    Cubic
};

enum class TextureType
{
    None = 0,
    Texture2D,
    TextureCube
};


namespace ImageUtils
{
    inline uint32_t GetImageFormatBPP(ImageFormat format)
    {
        switch (format)
        {
            case ImageFormat::RED8UN:
                return 1;
            case ImageFormat::RED8UI:
                return 1;
            case ImageFormat::RED16UI:
                return 2;
            case ImageFormat::RED32UI:
                return 4;
            case ImageFormat::RED32F:
                return 4;
            case ImageFormat::RGB:
            case ImageFormat::SRGB:
                return 3;
            case ImageFormat::RGBA:
                return 4;
            case ImageFormat::RGBA16F:
                return 2 * 4;
            case ImageFormat::RGBA32F:
                return 4 * 4;
            case ImageFormat::B10R11G11UF:
                return 4;
        }
        assert(false);
    }

    inline bool IsIntegerBased(const ImageFormat format)
    {
        switch (format)
        {
            case ImageFormat::RED16UI:
            case ImageFormat::RED32UI:
            case ImageFormat::RED8UI:
            case ImageFormat::DEPTH32FSTENCIL8UINT:
                return true;
            case ImageFormat::DEPTH32F:
            case ImageFormat::RED8UN:
            case ImageFormat::RGBA32F:
            case ImageFormat::B10R11G11UF:
            case ImageFormat::RG16F:
            case ImageFormat::RG32F:
            case ImageFormat::RED32F:
            case ImageFormat::RG8:
            case ImageFormat::RGBA:
            case ImageFormat::RGBA16F:
            case ImageFormat::RGB:
            case ImageFormat::SRGB:
            case ImageFormat::DEPTH24STENCIL8:
                return false;
        }
        assert(false);
    }

    inline VkFormat VulkanImageFormat(ImageFormat format)
    {
        switch (format)
        {
            case ImageFormat::RED8UN:
                return VK_FORMAT_R8_UNORM;
            case ImageFormat::RED8UI:
                return VK_FORMAT_R8_UINT;
            case ImageFormat::RED16UI:
                return VK_FORMAT_R16_UINT;
            case ImageFormat::RED32UI:
                return VK_FORMAT_R32_UINT;
            case ImageFormat::RED32F:
                return VK_FORMAT_R32_SFLOAT;
            case ImageFormat::RG8:
                return VK_FORMAT_R8G8_UNORM;
            case ImageFormat::RG16F:
                return VK_FORMAT_R16G16_SFLOAT;
            case ImageFormat::RG32F:
                return VK_FORMAT_R32G32_SFLOAT;
            case ImageFormat::RGBA:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case ImageFormat::RGBA16F:
                return VK_FORMAT_R16G16B16A16_SFLOAT;
            case ImageFormat::RGBA32F:
                return VK_FORMAT_R32G32B32A32_SFLOAT;
            case ImageFormat::B10R11G11UF:
                return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
            case ImageFormat::DEPTH32FSTENCIL8UINT:
                return VK_FORMAT_D32_SFLOAT_S8_UINT;
            case ImageFormat::DEPTH32F:
                return VK_FORMAT_D32_SFLOAT;
            case ImageFormat::DEPTH24STENCIL8:
                return VK_FORMAT_D32_SFLOAT;    // TODO:: Use device depth format.
        }
        assert(false);
        return VK_FORMAT_UNDEFINED;
    }

    inline ImageFormat VulkanFormatToImageFormat(VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_R8_UNORM:
                return ImageFormat::RED8UN;
            case VK_FORMAT_R8_UINT:
                return ImageFormat::RED8UI;
            case VK_FORMAT_R16_UINT:
                return ImageFormat::RED16UI;
            case VK_FORMAT_R32_UINT:
                return ImageFormat::RED32UI;
            case VK_FORMAT_R32_SFLOAT:
                return ImageFormat::RED32F;
            case VK_FORMAT_R8G8_UNORM:
                return ImageFormat::RG8;
            case VK_FORMAT_R16G16_SFLOAT:
                return ImageFormat::RG16F;
            case VK_FORMAT_R32G32_SFLOAT:
                return ImageFormat::RG32F;
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_UNORM:
                return ImageFormat::RGBA;
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                return ImageFormat::RGBA16F;
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return ImageFormat::RGBA32F;
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
                return ImageFormat::B10R11G11UF;
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return ImageFormat::DEPTH32FSTENCIL8UINT;
            case VK_FORMAT_D32_SFLOAT:
                // Note: This could be either DEPTH32F or DEPTH24STENCIL8
                return ImageFormat::DEPTH32F;
        }
        assert(false && "Unsupported Vulkan format");
        return ImageFormat::None;
    }

    inline uint32_t CalculateMipCount(uint32_t width, uint32_t height)
    {
        return (uint32_t) glm::floor(glm::log2(glm::min(width, height))) + 1;
    }

    inline uint32_t GetImageMemorySize(ImageFormat format, uint32_t width, uint32_t height)
    {
        return width * height * GetImageFormatBPP(format);
    }

    inline bool IsDepthFormat(ImageFormat format)
    {
        if (format == ImageFormat::DEPTH24STENCIL8 || format == ImageFormat::DEPTH32F ||
            format == ImageFormat::DEPTH32FSTENCIL8UINT)
            return true;

        return false;
    }

    void InsertImageMemoryBarrier(
            VkCommandBuffer cmdbuffer,
            VkImage image,
            VkAccessFlags srcAccessMask,
            VkAccessFlags dstAccessMask,
            VkImageLayout oldImageLayout,
            VkImageLayout newImageLayout,
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkImageSubresourceRange subresourceRange);

    void SetImageLayout(
            VkCommandBuffer cmdbuffer,
            VkImage image,
            VkImageLayout oldImageLayout,
            VkImageLayout newImageLayout,
            VkImageSubresourceRange subresourceRange,
            VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    void SetImageLayout(
            VkCommandBuffer cmdbuffer,
            VkImage image,
            VkImageAspectFlags aspectMask,
            VkImageLayout oldImageLayout,
            VkImageLayout newImageLayout,
            VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
}
