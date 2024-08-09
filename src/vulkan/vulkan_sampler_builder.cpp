#include "vulkan_sampler_builder.h"
#include "vulkan_context.h"
#include "vulkan_utils.h"

VulkanSamplerBuilder::VulkanSamplerBuilder()
{
	m_CreateInfo = {};
	m_CreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	m_CreateInfo.anisotropyEnable = VK_FALSE;
	m_CreateInfo.maxAnisotropy = 1.0f;
	m_CreateInfo.magFilter = VK_FILTER_LINEAR;
	m_CreateInfo.minFilter = VK_FILTER_LINEAR;
	m_CreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	m_CreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	m_CreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	m_CreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	m_CreateInfo.mipLodBias = 0.0f;
	m_CreateInfo.minLod = 0.0f;
	m_CreateInfo.maxLod = 100.0f;
	m_CreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
}

VulkanSamplerBuilder& VulkanSamplerBuilder::SetAnisotropy(float anisotropy)
{
	m_CreateInfo.anisotropyEnable = anisotropy > 1.0f ? VK_TRUE : VK_FALSE;
	m_CreateInfo.maxAnisotropy = anisotropy;
	return *this;
}

VulkanSamplerBuilder& VulkanSamplerBuilder::SetFilter(VkFilter magFilter, VkFilter minFilter)
{
	m_CreateInfo.magFilter = magFilter;
	m_CreateInfo.minFilter = minFilter;
	return *this;
}

VulkanSamplerBuilder& VulkanSamplerBuilder::SetMipmapMode(VkSamplerMipmapMode mipmapMode)
{
	m_CreateInfo.mipmapMode = mipmapMode;
	return *this;
}

VulkanSamplerBuilder& VulkanSamplerBuilder::SetAddressMode(VkSamplerAddressMode addressMode)
{
	m_CreateInfo.addressModeU = addressMode;
	m_CreateInfo.addressModeV = addressMode;
	m_CreateInfo.addressModeW = addressMode;
	return *this;
}

VulkanSamplerBuilder& VulkanSamplerBuilder::SetAddressMode(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w)
{
	m_CreateInfo.addressModeU = u;
	m_CreateInfo.addressModeV = v;
	m_CreateInfo.addressModeW = w;
	return *this;
}

VulkanSamplerBuilder& VulkanSamplerBuilder::SetLodBias(float lodBias)
{
	m_CreateInfo.mipLodBias = lodBias;
	return *this;
}

VulkanSamplerBuilder& VulkanSamplerBuilder::SetLodRange(float minLod, float maxLod)
{
	m_CreateInfo.minLod = minLod;
	m_CreateInfo.maxLod = maxLod;
	return *this;
}

VulkanSamplerBuilder& VulkanSamplerBuilder::SetBorderColor(VkBorderColor borderColor)
{
	m_CreateInfo.borderColor = borderColor;
	return *this;
}

VulkanSamplerBuilder& VulkanSamplerBuilder::SetForIntegerFormat(bool isInteger)
{
	if (isInteger)
	{
		m_CreateInfo.magFilter = VK_FILTER_NEAREST;
		m_CreateInfo.minFilter = VK_FILTER_NEAREST;
		m_CreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}
	return *this;
}

VkSampler VulkanSamplerBuilder::Build()
{
	VkSampler sampler;
	VK_CHECK_RESULT(vkCreateSampler(VulkanContext::Get().Device(), &m_CreateInfo, nullptr, &sampler));
	return sampler;
}