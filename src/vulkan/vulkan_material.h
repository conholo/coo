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
    explicit VulkanMaterial(std::shared_ptr<VulkanMaterialLayout> layout);
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
    void SetPushConstant(const std::string& name, const T& value);
    void BindPushConstants(VkCommandBuffer commandBuffer);

    VkPipelineLayout GetPipelineLayout() const { return m_Layout->GetPipelineLayout(); }
    std::shared_ptr<VulkanMaterial> Clone() const;

private:
    void AllocateDescriptorSets();

    std::shared_ptr<VulkanMaterialLayout> m_Layout;
    std::unique_ptr<VulkanDescriptorPool> m_DescriptorPool;
    std::vector<std::vector<VkDescriptorSet>> m_DescriptorSets;
    std::unordered_map<std::string, std::vector<uint8_t>> m_PushConstantData;
};