#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class VulkanInstance
{
public:
    VkInstance Instance{};
    VkDebugUtilsMessengerEXT DebugMessenger{};

    VulkanInstance() = default;
    ~VulkanInstance() = default;
    void Initialize(const std::vector<const char*>& requestedValidationLayers, bool enableValidationLayers = false);

private:
    void CreateInstance(const std::vector<const char*>& requestedValidationLayers, bool enableValidationLayers);
    void SetupDebugMessenger(bool enableValidationLayers);
};