#pragma once

#include <vulkan/vulkan.h>
#include "vulkan_physical_device.h"

class VulkanLogicalDevice
{
public:
    VkDevice Device{};
    VkQueue m_GraphicsQueue{};
    VkQueue m_PresentQueue{};
    VkQueue m_ComputeQueue{};

    void Initialize(VulkanPhysicalDevice& physicalDeviceRef, const std::vector<const char *> &requestedDeviceExtensions);

    VulkanLogicalDevice() = default;
    ~VulkanLogicalDevice() = default;
    VulkanLogicalDevice(const VulkanLogicalDevice &) = delete;
    VulkanLogicalDevice& operator=(const VulkanLogicalDevice &) = delete;
    VulkanLogicalDevice(VulkanLogicalDevice&&) = delete;
    VulkanLogicalDevice &operator=(VulkanLogicalDevice &&) = delete;
};