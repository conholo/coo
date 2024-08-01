#include "vulkan_logical_device.h"

#include <vector>
#include <set>
#include <stdexcept>

void VulkanLogicalDevice::Initialize(VulkanPhysicalDevice& physicalDeviceRef, const std::vector<const char *> &requestedDeviceExtensions)
{
    QueueFamilyIndices indices = physicalDeviceRef.ReadQueueFamilyIndices();
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.GraphicsFamily.value(), indices.PresentFamily.value(),
                                              indices.ComputeFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily: uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requestedDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = requestedDeviceExtensions.data();

    if (vkCreateDevice(physicalDeviceRef.PhysicalDevice, &createInfo, nullptr, &Device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(Device, indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(Device, indices.PresentFamily.value(), 0, &m_PresentQueue);
    vkGetDeviceQueue(Device, indices.ComputeFamily.value(), 0, &m_ComputeQueue);
}