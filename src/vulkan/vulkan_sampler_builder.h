#pragma once

#include <vulkan/vulkan.h>

class VulkanSamplerBuilder
{
public:
	VulkanSamplerBuilder();

	VulkanSamplerBuilder& SetAnisotropy(float anisotropy);
	VulkanSamplerBuilder& SetFilter(VkFilter magFilter, VkFilter minFilter);
	VulkanSamplerBuilder& SetMipmapMode(VkSamplerMipmapMode mipmapMode);
	VulkanSamplerBuilder& SetAddressMode(VkSamplerAddressMode addressMode);
	VulkanSamplerBuilder& SetAddressMode(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w);
	VulkanSamplerBuilder& SetLodBias(float lodBias);
	VulkanSamplerBuilder& SetLodRange(float minLod, float maxLod);
	VulkanSamplerBuilder& SetBorderColor(VkBorderColor borderColor);
	VulkanSamplerBuilder& SetForIntegerFormat(bool isInteger);

	VkSampler Build();

private:
	VkSamplerCreateInfo m_CreateInfo{};
};