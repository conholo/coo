#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR> PresentModes;
};

struct QueueFamilyIndices
{
    std::optional<uint32_t> GraphicsFamily;
    std::optional<uint32_t> ComputeFamily;
    std::optional<uint32_t> PresentFamily;

    bool IsComplete() const { return GraphicsFamily.has_value() && PresentFamily.has_value() && ComputeFamily.has_value(); }
};

struct VulkanPhysicalDevice
{
public:
    VkPhysicalDevice PhysicalDevice{};
    VkPhysicalDeviceProperties PhysicalDeviceProperties{};

    VulkanPhysicalDevice() = default;
    ~VulkanPhysicalDevice() = default;
    void Initialize(VkPhysicalDevice physicalDevice);
    bool IsDeviceSuitable(VkSurfaceKHR surface, const std::vector<const char *>& requestedDeviceExtensions);
    uint32_t FindDeviceMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    QueueFamilyIndices ReadQueueFamilyIndices() const { return m_QueueFamilyIndices; }
    SwapchainSupportDetails ReadSwapchainSupportDetails() const { return m_SwapchainSupportDetails; }

private:
    bool CheckDeviceExtensionSupport(const std::vector<const char *>& deviceExtensions) const;
    void QuerySwapchainSupportDetails(VkSurfaceKHR surface);
    void QueryQueueFamilyIndices(VkSurfaceKHR surface);

private:
    QueueFamilyIndices m_QueueFamilyIndices{};
    SwapchainSupportDetails m_SwapchainSupportDetails{};
};