#include "vulkan_swapchain.h"

#include <utility>
#include "vulkan_context.h"
#include "vulkan_utils.h"

VulkanSwapchain::VulkanSwapchain(VkExtent2D windowExtent)
    : m_WindowExtent(windowExtent)
{
    Create();
}

VulkanSwapchain::VulkanSwapchain(VkExtent2D windowExtent, std::shared_ptr<VulkanSwapchain> previous)
    : m_WindowExtent(windowExtent), m_PreviousSwapchain{std::move(previous)}
{
    Create();
    m_PreviousSwapchain = nullptr;
}

VulkanSwapchain::~VulkanSwapchain()
{
    Destroy();
}

void VulkanSwapchain::Destroy()
{
    auto device = VulkanContext::Get().Device();

    for (auto &image: m_Images)
    {
        image->Release();
    }
    m_Images.clear();

    if (m_Swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
        m_Swapchain = VK_NULL_HANDLE;
    }
}

void VulkanSwapchain::Create()
{
    SwapchainSupportDetails swapChainSupport = VulkanContext::Get().GetAvailableDeviceSwapchainSupportDetails();

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

    uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
    if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.Capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = VulkanContext::Get().Surface();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = VulkanContext::Get().GetAvailableDeviceQueueFamilyIndices();
    uint32_t queueFamilyIndices[] = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

    if (indices.GraphicsFamily.value() != indices.PresentFamily.value())
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;      // Optional
        createInfo.pQueueFamilyIndices = nullptr;  // Optional
    }

    createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = m_PreviousSwapchain == nullptr ? VK_NULL_HANDLE : m_PreviousSwapchain->m_Swapchain;

    VK_CHECK_RESULT(vkCreateSwapchainKHR(VulkanContext::Get().Device(), &createInfo, nullptr, &m_Swapchain));

    m_SwapchainImageFormat = surfaceFormat.format;
    m_SwapchainExtent = extent;

    vkGetSwapchainImagesKHR(VulkanContext::Get().Device(), m_Swapchain, &imageCount, nullptr);
    std::vector<VkImage> swapChainImages(imageCount);
    vkGetSwapchainImagesKHR(VulkanContext::Get().Device(), m_Swapchain, &imageCount, swapChainImages.data());

    m_Images.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        ImageSpecification spec;
        spec.Width = m_SwapchainExtent.width;
        spec.Height = m_SwapchainExtent.height;
        spec.Format = ImageUtils::VulkanFormatToImageFormat(m_SwapchainImageFormat);
        spec.Usage = ImageUsage::Swapchain;
        spec.ExistingImage = swapChainImages[i];
        spec.SwapchainFormat = m_SwapchainImageFormat;
        m_Images[i] = std::make_shared<VulkanImage2D>(spec);
    }
}

VkResult VulkanSwapchain::AcquireNextImage(uint32_t *imageIndex, VkSemaphore imageAvailableSemaphore)
{
    VkResult result = vkAcquireNextImageKHR(
            VulkanContext::Get().Device(),
            m_Swapchain,
            std::numeric_limits<uint64_t>::max(),
            imageAvailableSemaphore,
            VK_NULL_HANDLE,
            imageIndex);

    return result;
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    for (const auto &availableFormat: availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    for (const auto &availablePresentMode: availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            std::cout << "Present mode: Mailbox" << std::endl;
            return availablePresentMode;
        }
    }

    std::cout << "Present mode: V-Sync" << std::endl;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = m_WindowExtent;
        actualExtent.width = std::max(
                capabilities.minImageExtent.width,
                std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(
                capabilities.minImageExtent.height,
                std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

