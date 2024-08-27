#pragma once

#include "vulkan_material_layout.h"
#include "vulkan_descriptors.h"
#include <memory>
#include <vector>

struct DescriptorUpdate
{
    uint32_t binding{};
    enum class Type { Buffer, Image } type{};
    union
    {
		VkDescriptorBufferInfo bufferInfo{};
		VkDescriptorImageInfo imageInfo;
	};
    // Optional fields for more specific updates
    VkDeviceSize offset = 0;
    VkDeviceSize range = VK_WHOLE_SIZE;
    uint8_t imageMip = 0;
};

class VulkanMaterial
{
public:
    explicit VulkanMaterial(VulkanMaterialLayout& layoutRef);
    ~VulkanMaterial();

    // Disable copying
    VulkanMaterial(const VulkanMaterial&) = delete;
    VulkanMaterial& operator=(const VulkanMaterial&) = delete;

    // Enable moving
    VulkanMaterial(VulkanMaterial&& other) noexcept;
    VulkanMaterial& operator=(VulkanMaterial&& other) noexcept;

    void BindDescriptors(uint32_t frameIndex, VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint);
    void UpdateDescriptor(uint32_t frameIndex, uint32_t set, const DescriptorUpdate& update);
    void UpdateDescriptorSets(uint32_t frameIndex, const std::vector<std::pair<uint32_t, std::vector<DescriptorUpdate>>>& updates);

	template<typename T>
	void SetPushConstant(const std::string& name, const T& value)
	{
		const auto& pushConstantRanges = m_LayoutRef.GetPushConstantRanges();
		auto it = std::find_if(pushConstantRanges.begin(), pushConstantRanges.end(), [&name](const VulkanMaterialLayout::PushConstantRange& range) { return range.name == name; });

		if (it == pushConstantRanges.end())
		{
			throw std::runtime_error("Push constant not found: " + name);
		}

		if (sizeof(T) > it->size)
		{
			throw std::runtime_error("Push constant data too large for: " + name);
		}

		std::vector<uint8_t> data(sizeof(T));
		memcpy(data.data(), &value, sizeof(T));
		m_PushConstantData[name] = std::move(data);
	}

    void BindPushConstants(VkCommandBuffer commandBuffer);

	VkDescriptorSet GetDescriptorSetAt(uint32_t frameIndex, uint32_t index) { return m_DescriptorSets[frameIndex][index]; }
    VkPipelineLayout GetPipelineLayout() const { return m_LayoutRef.GetPipelineLayout(); }
    std::shared_ptr<VulkanMaterial> Clone() const;

private:
    void AllocateDescriptorSets();

    VulkanMaterialLayout& m_LayoutRef;
    std::unique_ptr<VulkanDescriptorPool> m_DescriptorPool;
    std::vector<std::vector<VkDescriptorSet>> m_DescriptorSets;
    std::unordered_map<std::string, std::vector<uint8_t>> m_PushConstantData;
};