#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <vector>

class VulkanDescriptorSetLayout
{
public:
    class Builder
    {
    public:
        Builder() = default;
        Builder& AddDescriptor(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1);
        [[nodiscard]] std::unique_ptr<VulkanDescriptorSetLayout> Build() const;

    private:
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings{};
    };

    explicit VulkanDescriptorSetLayout(const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& bindings);
    ~VulkanDescriptorSetLayout();

    VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
    VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;
    std::vector<VkDescriptorSetLayoutBinding> GetDescriptors() const;
    VkDescriptorSetLayout& GetDescriptorSetLayout() { return m_DescriptorSetLayout; }

private:
    VkDescriptorSetLayout m_DescriptorSetLayout{};
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Descriptors;

    friend class VulkanDescriptorWriter;
};

class VulkanDescriptorPool
{
public:
    class Builder
    {
    public:
        Builder() = default;

        Builder& AddPoolSize(VkDescriptorType descriptorType, uint32_t count);
        Builder& SetPoolFlags(VkDescriptorPoolCreateFlags flags);
        Builder& SetMaxSets(uint32_t count);
        std::unique_ptr<VulkanDescriptorPool> Build() const;
		bool IsNotEmpty() const { return !m_PoolSizes.empty(); }

		private:
        std::vector<VkDescriptorPoolSize> m_PoolSizes{};
        uint32_t m_MaxSets = 1000;
        VkDescriptorPoolCreateFlags m_PoolFlags = 0;
    };

    VulkanDescriptorPool(
        uint32_t m_MaxSets,
        VkDescriptorPoolCreateFlags m_PoolFlags,
        const std::vector<VkDescriptorPoolSize>& m_PoolSizes);

    ~VulkanDescriptorPool();

    VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
    VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;

    bool AllocateDescriptor(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;
    void FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;
    void ResetPool();

private:
    VkDescriptorPool m_DescriptorPool{};

    friend class VulkanDescriptorWriter;
};

class VulkanDescriptorWriter
{
public:
    VulkanDescriptorWriter(VulkanDescriptorSetLayout& setLayout, VulkanDescriptorPool& pool);

    VulkanDescriptorWriter& WriteBuffer(uint32_t binding, const VkDescriptorBufferInfo& bufferInfo);
    VulkanDescriptorWriter& WriteImage(uint32_t binding, const VkDescriptorImageInfo& imageInfo);

    bool Build(VkDescriptorSet& set);
    void Overwrite(VkDescriptorSet& set);

private:
    VulkanDescriptorSetLayout& m_SetLayout;
    VulkanDescriptorPool& m_Pool;
    std::vector<VkWriteDescriptorSet> m_Writes;
};
