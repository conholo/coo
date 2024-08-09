#include "vulkan_context.h"
#include "vulkan_utils.h"
#include <iostream>

void VulkanContext::Initialize(const char *applicationName, uint32_t applicationVersion, Window *windowPtr)
{
    VulkanContext& instance = Get();
    if (m_Initialized) return;

    m_VkAppInfo.pApplicationName = applicationName;
    m_VkAppInfo.applicationVersion = applicationVersion;
    m_WindowPtr = windowPtr;
    instance.CreateContext();
}

void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}

void VulkanContext::Shutdown()
{
    VulkanContext &instance = Get();

    if (instance.m_GraphicsCommandPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(instance.m_LogicalDevice.Device, instance.m_GraphicsCommandPool, nullptr);
    if (instance.m_ComputeCommandPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(instance.m_LogicalDevice.Device, instance.m_ComputeCommandPool, nullptr);

    vkDestroyDevice(instance.m_LogicalDevice.Device, nullptr);

    if (instance.m_EnableValidationLayers)
        DestroyDebugUtilsMessengerEXT(instance.m_Instance.Instance, instance.m_Instance.DebugMessenger, nullptr);

    vkDestroySurfaceKHR(instance.m_Instance.Instance, instance.m_Surface, nullptr);
    vkDestroyInstance(instance.m_Instance.Instance, nullptr);
}

void VulkanContext::CreateContext()
{
    if (m_Initialized) return;

    m_Instance.Initialize(m_ValidationLayers, m_EnableValidationLayers);
    CreateSurface(*m_WindowPtr);
    SelectPhysicalDevice();
    QueueFamilyIndices indices = m_PhysicalDevice.ReadQueueFamilyIndices();
    m_QueueFamilyIndices = { indices.GraphicsFamily.value(), indices.ComputeFamily.value() };
    m_LogicalDevice.Initialize(m_PhysicalDevice, m_DeviceExtensions);
    CreateGraphicsCommandPool();
    CreateComputeCommandPool();

    m_Initialized = true;
}

void VulkanContext::CreateSurface(Window& windowRef)
{
    windowRef.CreateWindowSurface(m_Instance.Instance, &m_Surface);
}

void VulkanContext::SelectPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance.Instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");

    std::cout << "Device count: " << deviceCount << std::endl;
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance.Instance, &deviceCount, devices.data());

    // Create wrappers and initialize them.
    for(const auto& device: devices)
    {
        VulkanPhysicalDevice physicalDevice;
        physicalDevice.Initialize(device);
        m_PhysicalDevices.push_back(physicalDevice);
    }

    // Query for a suitable device.
    bool foundSuitableDevice = false;
    for (auto &device: m_PhysicalDevices)
    {
        if (device.IsDeviceSuitable(m_Surface, m_DeviceExtensions))
        {
            m_PhysicalDevice = device;
            foundSuitableDevice = true;
            break;
        }
    }

    if (!foundSuitableDevice)
        throw std::runtime_error("Failed to find a suitable physical device!");

    std::cout << "Physical device: " << m_PhysicalDevice.PhysicalDeviceProperties.deviceName << std::endl;
}


void VulkanContext::CreateGraphicsCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = m_PhysicalDevice.ReadQueueFamilyIndices();
    if (!queueFamilyIndices.GraphicsFamily.has_value())
    {
        std::cerr << "Unable to CreateGraphicsCommandPool(), no GraphicsFamily indices were found." << "\n";
        return;
    }

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_LogicalDevice.Device, &poolInfo, nullptr, &m_GraphicsCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics command pool!");
    }
}

void VulkanContext::CreateComputeCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = m_PhysicalDevice.ReadQueueFamilyIndices();

    if(!queueFamilyIndices.ComputeFamily.has_value())
    {
        std::cerr << "Unable to CreateComputeCommandPool(), no ComputeFamily indices were found." << "\n";
        return;
    }

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.ComputeFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_LogicalDevice.Device, &poolInfo, nullptr, &m_ComputeCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create compute command pool!");
    }
}

VkCommandBuffer VulkanContext::BeginSingleTimeCommands(QueueFamilyType family)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkCommandPool pool = VK_NULL_HANDLE;
    switch(family)
    {
        case QueueFamilyType::Compute:
        {
            pool = m_ComputeCommandPool;
            break;
        }
        case QueueFamilyType::Present:
        case QueueFamilyType::Graphics:
        default:
        {
            pool = m_GraphicsCommandPool;
            break;
        }
    }

    allocInfo.commandPool = pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_LogicalDevice.Device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void VulkanContext::EndSingleTimeCommand(VkCommandBuffer commandBuffer, QueueFamilyType family)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkQueue queue = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;
    switch (family)
    {
        case QueueFamilyType::Compute:
        {
            queue = m_LogicalDevice.m_ComputeQueue;
            pool = m_ComputeCommandPool;
            break;
        }
        case QueueFamilyType::Present:
        case QueueFamilyType::Graphics:
        default:
        {
            queue = m_LogicalDevice.m_GraphicsQueue;
            pool = m_GraphicsCommandPool;
            break;
        }
    }

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(m_LogicalDevice.Device, pool, 1, &commandBuffer);
}

uint32_t VulkanContext::FindDeviceMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    return m_PhysicalDevice.FindDeviceMemoryType(typeFilter, properties);
}

VkFormat VulkanContext::SelectSupportedFormat(
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features) const
{
    for (VkFormat format: candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice.PhysicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }
    throw std::runtime_error("Failed to find supported format!");
}

SwapchainSupportDetails VulkanContext::QuerySwapchainSupportDetailsOnSwapchainRecreation()
{
	m_PhysicalDevice.QuerySwapchainSupportDetails(m_Surface);
	return m_PhysicalDevice.m_SwapchainSupportDetails;
}
