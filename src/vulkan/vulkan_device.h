#pragma once

#include "core/window.h"
#include <string>
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

    [[nodiscard]] bool IsComplete() const { return GraphicsFamily.has_value() && PresentFamily.has_value() && ComputeFamily.has_value(); }
};


class VulkanDevice
{
public:
    explicit VulkanDevice(Window& window);
    ~VulkanDevice();

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;
    VulkanDevice(VulkanDevice&&) = delete;
    VulkanDevice& operator=(VulkanDevice&&) = delete;

    VkCommandPool GetGraphicsCommandPool() { return m_GraphicsCommandPool; }
    VkCommandPool GetComputeCommandPool() { return m_ComputeCommandPool; }
    VkDevice GetDevice() { return m_LogicalDevice; }
    VkSurfaceKHR GetSurface() { return m_Surface; }
    VkQueue GetGraphicsQueue() { return m_GraphicsQueue; }
    VkQueue GetPresentQueue() { return m_PresentQueue; }
    VkQueue GetComputeQueue() { return m_ComputeQueue; }
    VkPhysicalDevice GetPhysicalDevice() { return m_PhysicalDevice; }

    SwapchainSupportDetails GetSwapchainSupport() { return QuerySwapchainSupport(m_PhysicalDevice); }
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    QueueFamilyIndices FindPhysicalQueueFamilies() { return FindQueueFamilies(m_PhysicalDevice); }
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

    VkPhysicalDeviceProperties PhysicalDeviceProperties{};

    void CreateImageWithInfo(const VkImageCreateInfo& imageInfo,
                             VkMemoryPropertyFlags properties,
                             VkImage& image,
                             VkDeviceMemory& imageMemory);

    void CreateBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
            VkBuffer &buffer, VkDeviceMemory &bufferMemory);

    void CopyBuffer(
        VkBuffer srcBuffer, VkBuffer dstBuffer,
        VkDeviceSize size);

    void CopyBufferToImage(
        VkBuffer buffer,
        VkImage image,
        uint32_t width, uint32_t height,
        uint32_t layerCount);

    void TransitionImageLayout(
        VkImage image,
        VkFormat format,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        uint32_t mipLevels = 1,
        uint32_t layerCount = 1);

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommand(VkCommandBuffer commandBuffer);

private:
    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void SelectPhysicalDevice();
    void CreateLogicalDevice();
    void CreateGraphicsCommandPool();
    void CreateComputeCommandPool();

    bool IsDeviceSuitable(VkPhysicalDevice device);
    [[nodiscard]] std::vector<const char*> GetRequiredExtensions() const;
    bool CheckValidationLayerSupport();

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    void HasGLFWRequiredInstanceExtensions();
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

    SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);

    Window& m_WindowRef;

    bool m_EnableValidationLayers = true;
    VkInstance m_Instance{};
    VkDebugUtilsMessengerEXT m_DebugMessenger{};
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_GraphicsCommandPool{};
    VkCommandPool m_ComputeCommandPool{};

    VkDevice m_LogicalDevice{};
    VkSurfaceKHR m_Surface{};
    VkQueue m_GraphicsQueue{};
    VkQueue m_PresentQueue{};
    VkQueue m_ComputeQueue{};

    const std::vector<const char *> m_ValidationLayers = {
            "VK_LAYER_KHRONOS_validation",
            //"VK_LAYER_LUNARG_api_dump",
            //"VK_LAYER_KHRONOS_profiles",
            //"VK_LAYER_KHRONOS_synchronization2"
    };

    const std::vector<const char *> m_DeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};