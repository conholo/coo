#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <unordered_map>

#include "vulkan_instance.h"
#include "vulkan_physical_device.h"
#include "core/uuid.h"
#include "core/window.h"
#include "vulkan_logical_device.h"

enum class QueueFamilyType
{
    None = 0,
    Graphics = 1,
    Compute = 2,
    Present = 3
};

class VulkanContext
{
public:

    static VulkanContext& Get()
    {
        static VulkanContext context;
        return context;
    }

    static void Initialize(const char *applicationName, uint32_t applicationVersion, Window* windowPtr);
    static void Shutdown();

    VkDevice Device() const { return m_LogicalDevice.Device; }
    VkPhysicalDevice PhysicalDevice() const { return m_PhysicalDevice.PhysicalDevice; }
    VkPhysicalDeviceProperties PhysicalDeviceProperties() const { return m_PhysicalDevice.PhysicalDeviceProperties; }

    VkSurfaceKHR Surface() const { return  m_Surface; }
    VkCommandPool GraphicsCommandPool() const { return m_GraphicsCommandPool; }
    VkCommandPool ComputeCommandPool() const { return m_GraphicsCommandPool; }

    QueueFamilyIndices GetAvailableDeviceQueueFamilyIndices() const { return m_PhysicalDevice.ReadQueueFamilyIndices(); }
    SwapchainSupportDetails GetAvailableDeviceSwapchainSupportDetails() const { return m_PhysicalDevice.ReadSwapchainSupportDetails(); }

    VkQueue GraphicsQueue() const { return m_LogicalDevice.m_GraphicsQueue; }
    VkQueue PresentQueue() const { return m_LogicalDevice.m_PresentQueue; }
    VkQueue ComputeQueue() const { return m_LogicalDevice.m_ComputeQueue; }

public:
    VkCommandBuffer BeginSingleTimeCommands(QueueFamilyType family = QueueFamilyType::Graphics);
    void EndSingleTimeCommand(VkCommandBuffer commandBuffer, QueueFamilyType family = QueueFamilyType::Graphics);
    uint32_t FindDeviceMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkFormat SelectSupportedFormat(
            const std::vector<VkFormat> &candidates,
            VkImageTiling tiling,
            VkFormatFeatureFlags features) const;

private:
    VulkanContext() = default;
    ~VulkanContext() = default;

    void CreateContext();
    void CreateSurface(Window& windowRef);
    void SelectPhysicalDevice();

    void CreateGraphicsCommandPool();
    void CreateComputeCommandPool();
private:
    inline static bool m_Initialized = false;
    inline static VkApplicationInfo m_VkAppInfo{};
    inline static Window* m_WindowPtr = nullptr;

    VkSurfaceKHR m_Surface{};

    VulkanInstance m_Instance{};
    VulkanPhysicalDevice m_PhysicalDevice;
    std::vector<VulkanPhysicalDevice> m_PhysicalDevices{};
    VulkanLogicalDevice m_LogicalDevice;

    VkCommandPool m_GraphicsCommandPool{};
    VkCommandPool m_ComputeCommandPool{};
    std::vector<uint32_t> m_QueueFamilyIndices{};

    const std::vector<const char *> m_DeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    const std::vector<const char *> m_ValidationLayers =
    {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_LUNARG_api_dump",
        "VK_LAYER_KHRONOS_profiles",
    };

#if NDEBUG
    bool m_EnableValidationLayers = false;
#else
    bool m_EnableValidationLayers = true;
#endif

};