#include <iostream>
#include "vulkan_material.h"
#include "vulkan_descriptors.h"
#include "vulkan_context.h"

VulkanMaterial::VulkanMaterial(std::shared_ptr<VulkanShader> vertexShader, std::shared_ptr<VulkanShader> fragmentShader)
    :m_VertexShader(vertexShader), m_FragmentShader(fragmentShader)
{
    AllocateDescriptorSets();
    InitializePushConstants();
    CreatePipelineLayout();
}

void VulkanMaterial::CreatePipelineLayout()
{
    // Prepare descriptor set layouts
    std::vector<VkDescriptorSetLayout> setLayouts(m_DescriptorSetsLayouts.size());
    for(uint32_t i = 0; i < m_DescriptorSetsLayouts.size(); ++i)
        setLayouts[i] = m_DescriptorSetsLayouts[i]->GetDescriptorSetLayout();

    // Prepare push constant ranges
    std::vector<VkPushConstantRange> pushConstantRanges;
    uint32_t offset = 0;
    for (const auto& block : m_PushConstantBlocks)
    {
        VkPushConstantRange range{};
        range.stageFlags = block.stageFlags;
        range.offset = offset;
        range.size = static_cast<uint32_t>(block.data.size());
        pushConstantRanges.push_back(range);
        offset += range.size;
    }

    // Create the pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

    if (vkCreatePipelineLayout(VulkanContext::Get().Device(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void VulkanMaterial::CreateLayoutsAndAllocateDescriptorSets()
{
    uint32_t maxSet = *m_ShaderDescriptorInfo.uniqueSets.rbegin();

    m_DescriptorSets.resize(maxSet + 1);
    m_DescriptorSetsLayouts.resize(maxSet + 1);

    for (uint32_t set : m_ShaderDescriptorInfo.uniqueSets)
    {
        VulkanDescriptorSetLayout::Builder layoutBuilder;

        for (const auto& descriptorInfo : m_ShaderDescriptorInfo.setDescriptors[set])
        {
            layoutBuilder.AddDescriptor(
                    descriptorInfo.binding,
                    descriptorInfo.type,
                    descriptorInfo.stageFlags,
                    descriptorInfo.count
            );
        }

        m_DescriptorSetsLayouts[set] = layoutBuilder.Build();

        if (!m_DescriptorSets[set])
            m_DescriptorSets[set] = std::make_shared<VkDescriptorSet>(VK_NULL_HANDLE);

        VkDescriptorSet& setRef = *m_DescriptorSets[set];
        VulkanDescriptorWriter(*m_DescriptorSetsLayouts[set], *m_DescriptorPool)
                .Build(setRef);
    }
}

void VulkanMaterial::AllocateDescriptorSets()
{
    // Populate shader descriptor info
    m_ShaderDescriptorInfo.AddShaderReflection(m_VertexShader->GetReflection(), m_VertexShader->GetShaderStage());
    m_ShaderDescriptorInfo.AddShaderReflection(m_FragmentShader->GetReflection(), m_FragmentShader->GetShaderStage());

    VulkanDescriptorPool::Builder poolBuilder;

    // Add pool sizes
    for (const auto& [type, count] : m_ShaderDescriptorInfo.totalDescriptorCounts)
    {
        poolBuilder.AddPoolSize(type, count);
    }

    // Set the max sets to the total number of unique sets
    poolBuilder.SetMaxSets(m_ShaderDescriptorInfo.GetTotalUniqueSetCount());

    m_DescriptorPool = poolBuilder.Build();
    CreateLayoutsAndAllocateDescriptorSets();
}

void VulkanMaterial::BindDescriptors(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint)
{
    std::vector<VkDescriptorSet> descriptorSets;
    descriptorSets.reserve(m_DescriptorSets.size());

    for (const auto& descriptorSet : m_DescriptorSets)
    {
        descriptorSets.push_back(*descriptorSet);
    }

    vkCmdBindDescriptorSets(
            commandBuffer,
            pipelineBindPoint,
            m_PipelineLayout,
            0,
            static_cast<uint32_t>(descriptorSets.size()),
            descriptorSets.data(),
            0,
            nullptr
    );
}


void VulkanMaterial::UpdateDescriptor(uint32_t set, const DescriptorUpdate& update)
{
    UpdateDescriptorSets(
            {
                {set, {update}}
            });
}

void VulkanMaterial::UpdateDescriptorSets(const std::vector<std::pair<uint32_t, std::vector<DescriptorUpdate>>>& updates)
{
    for (const auto& [set, descriptorUpdates] : updates)
    {
        if (set >= m_DescriptorSets.size())
        {
            throw std::runtime_error("Descriptor set index out of range");
        }

        VulkanDescriptorWriter writer(*m_DescriptorSetsLayouts[set], *m_DescriptorPool);

        for (const auto& update : descriptorUpdates)
        {
            switch (update.type)
            {
                case DescriptorUpdate::Type::Buffer:
                {
                    writer.WriteBuffer(update.binding, &update.bufferInfo);
                    break;
                }
                case DescriptorUpdate::Type::Image:
                {
                    writer.WriteImage(update.binding, &update.imageInfo);
                    break;
                }
            }
        }

        writer.Overwrite(*m_DescriptorSets[set]);
    }
}

void VulkanMaterial::BindPushConstants(VkCommandBuffer commandBuffer)
{
    for (const auto& block : m_PushConstantBlocks)
    {
        vkCmdPushConstants(
                commandBuffer,
                m_PipelineLayout,
                block.stageFlags,
                0,
                static_cast<uint32_t>(block.data.size()),
                block.data.data()
        );
    }
}

void VulkanMaterial::InitializePushConstants()
{
    auto addPushConstants = [this](const VulkanShaderReflection& reflection)
    {
        for (const auto& pc : reflection.GetPushConstantRanges())
        {
            PushConstantBlock block;
            block.name = pc.name;
            block.stageFlags = pc.stageFlags;
            block.data.resize(pc.size);

            m_PushConstantBlocks.push_back(block);
            m_PushConstantBlockIndex[pc.name] = m_PushConstantBlocks.size() - 1;
        }
    };

    addPushConstants(m_VertexShader->GetReflection());
    addPushConstants(m_FragmentShader->GetReflection());
}

template<typename T>
void VulkanMaterial::SetPushConstant(const std::string &name, const T &value)
{
    auto it = m_PushConstantBlockIndex.find(name);
    if (it == m_PushConstantBlockIndex.end())
    {
        throw std::runtime_error("Push constant not found: " + name);
    }

    PushConstantBlock& block = m_PushConstantBlocks[it->second];
    if (sizeof(T) > block.data.size())
    {
        throw std::runtime_error("Push constant data too large for: " + name);
    }

    memcpy(block.data.data(), &value, sizeof(T));
}
