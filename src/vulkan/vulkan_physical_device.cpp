#include "vulkan_physical_device.h"
#include <set>
#include <cassert>

void VulkanPhysicalDevice::Initialize(VkPhysicalDevice physicalDevice)
{
    PhysicalDevice = physicalDevice;
}

bool VulkanPhysicalDevice::IsDeviceSuitable(VkSurfaceKHR surface, const std::vector<const char *> &requestedDeviceExtensions)
{
    assert(PhysicalDevice != VK_NULL_HANDLE && "VulkanPhysicalDevice was not Initialized() with a valid VkPhysicalDevice.");

    QueryQueueFamilyIndices(surface);

    bool extensionsSupported = CheckDeviceExtensionSupport(requestedDeviceExtensions);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        QuerySwapchainSupportDetails(surface);
        swapChainAdequate = !m_SwapchainSupportDetails.Formats.empty() && !m_SwapchainSupportDetails.PresentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(PhysicalDevice, &supportedFeatures);
    vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);

    return m_QueueFamilyIndices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool VulkanPhysicalDevice::CheckDeviceExtensionSupport(const std::vector<const char *>& deviceExtensions) const
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(
            PhysicalDevice,
            nullptr,
            &extensionCount,
            availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension: availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

void VulkanPhysicalDevice::QueryQueueFamilyIndices(VkSurfaceKHR surface)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily: queueFamilies)
    {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            m_QueueFamilyIndices.GraphicsFamily = i;

        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            m_QueueFamilyIndices.ComputeFamily = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, i, surface, &presentSupport);

        if (queueFamily.queueCount > 0 && presentSupport)
            m_QueueFamilyIndices.PresentFamily = i;

        if (m_QueueFamilyIndices.IsComplete())
            break;

        i++;
    }
}

void VulkanPhysicalDevice::QuerySwapchainSupportDetails(VkSurfaceKHR surface)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, surface, &m_SwapchainSupportDetails.Capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        m_SwapchainSupportDetails.Formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, surface, &formatCount, m_SwapchainSupportDetails.Formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        m_SwapchainSupportDetails.PresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
                PhysicalDevice,
                surface,
                &presentModeCount,
                m_SwapchainSupportDetails.PresentModes.data());
    }
}

uint32_t VulkanPhysicalDevice::FindDeviceMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}
