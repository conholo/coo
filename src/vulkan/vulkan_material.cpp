#include "vulkan_material.h"
#include "vulkan_descriptors.h"
#include "vulkan_context.h"

VulkanMaterial::VulkanMaterial(std::shared_ptr<VulkanShader> vertexShader, std::shared_ptr<VulkanShader> fragmentShader)
    :m_VertexShader(vertexShader), m_FragmentShader(fragmentShader)
{
    CreateDescriptorSetLayout();
    AllocateDescriptorSets();
    CreatePipelineLayout();
}

void VulkanMaterial::CreatePipelineLayout()
{
    std::vector<VkPushConstantRange> pushConstantRanges;

    auto addPushConstants = [&pushConstantRanges](const std::vector<PushConstantRange> &shaderPushConstants)
    {
        for (const auto &pc: shaderPushConstants)
        {
            VkPushConstantRange range{};
            range.stageFlags = pc.stageFlags;
            range.offset = pc.offset;
            range.size = pc.size;
            pushConstantRanges.push_back(range);
        }
    };

    addPushConstants(m_VertexShader->GetReflection().GetPushConstantRanges());
    addPushConstants(m_FragmentShader->GetReflection().GetPushConstantRanges());

    // Merge overlapping push constant ranges
    std::sort(pushConstantRanges.begin(), pushConstantRanges.end(),
              [](const VkPushConstantRange &a, const VkPushConstantRange &b)
              { return a.offset < b.offset; });

    std::vector<VkPushConstantRange> mergedRanges;
    for (const auto &range: pushConstantRanges)
    {
        bool doesNotOverlap = range.offset >= mergedRanges.back().offset + mergedRanges.back().size;
        if (mergedRanges.empty() || doesNotOverlap)
        {
            mergedRanges.push_back(range);
        }
        else
        {
            // Merge
            auto& last = mergedRanges.back();
            last.size = std::max(last.size, range.offset + range.size - last.offset);
            last.stageFlags |= range.stageFlags;
        }
    }

    m_PushConstantRanges = mergedRanges;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout->GetDescriptorSetLayout();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(mergedRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = mergedRanges.data();

    if (vkCreatePipelineLayout(VulkanContext::Get().Device(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void VulkanMaterial::SetPushConstants(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *data)
{
    // Find the corresponding push constant range
    auto it =
            std::find_if(m_PushConstantRanges.begin(), m_PushConstantRanges.end(),
               [stageFlags, offset, size](const VkPushConstantRange &range)
               {
                   return (range.stageFlags & stageFlags) &&
                          offset >= range.offset &&
                           offset + size <= range.offset + range.size;
               });

    if (it == m_PushConstantRanges.end())
    {
        throw std::runtime_error("No matching push constant range found");
    }

    // Resize m_PushConstantData if necessary
    if (offset + size > m_PushConstantData.size())
    {
        m_PushConstantData.resize(offset + size);
    }

    memcpy(m_PushConstantData.data() + offset, data, size);
}

void VulkanMaterial::CreateDescriptorSetLayout()
{
    VulkanDescriptorSetLayout::Builder builder;

    // Add bindings from vertex shader
    for (const auto &resource: m_VertexShader->GetReflection().GetResources())
    {
        builder.AddBinding(resource.binding, resource.descriptorType, resource.stageFlags, resource.arraySize);
    }

    // Add bindings from fragment shader
    for (const auto &resource: m_FragmentShader->GetReflection().GetResources())
    {
        builder.AddBinding(resource.binding, resource.descriptorType, resource.stageFlags, resource.arraySize);
    }

    m_DescriptorSetLayout = builder.Build();
}

void VulkanMaterial::AllocateDescriptorSets()
{
    VulkanDescriptorPool::Builder poolBuilder;
    // Add pool sizes based on shader reflection data
    m_DescriptorPool = poolBuilder.Build();

    VkDescriptorSet descriptorSet;
    m_DescriptorPool->AllocateDescriptor(m_DescriptorSetLayout->GetDescriptorSetLayout(), descriptorSet);
    m_DescriptorSets.push_back(descriptorSet);
}

void VulkanMaterial::UpdateDescriptorSet(uint32_t set, const std::vector<VkWriteDescriptorSet>& writes)
{
    if (set >= m_DescriptorSets.size())
    {
        throw std::runtime_error("Descriptor set index out of range");
    }

    VulkanDescriptorWriter writer(*m_DescriptorSetLayout, *m_DescriptorPool);

    for (auto write: writes)
    {
        switch (write.descriptorType)
        {
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                writer.WriteBuffer(write.dstBinding, write.pBufferInfo);
                break;
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                writer.WriteImage(write.dstBinding, write.pImageInfo);
                break;
            default:
                throw std::runtime_error("Unsupported descriptor type");
        }
    }

    writer.Overwrite(m_DescriptorSets[set]);
}

void VulkanMaterial::UpdateDescriptorSets(const std::vector<std::pair<uint32_t, std::vector<VkWriteDescriptorSet>>> &updates)
{
    for (const auto &[set, writes]: updates)
    {
        UpdateDescriptorSet(set, writes);
    }
}