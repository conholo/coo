#include "vulkan_material_layout.h"

#include "vulkan_context.h"
#include "vulkan_utils.h"

#include <utility>

VulkanMaterialLayout::VulkanMaterialLayout(VulkanShader& vertexShader, VulkanShader& fragmentShader, std::string debugName)
        : m_DebugName(std::move(debugName))
{
    m_ShaderDescriptorInfo.AddShaderReflection(vertexShader.GetReflection(), vertexShader.GetShaderStage());
    m_ShaderDescriptorInfo.AddShaderReflection(fragmentShader.GetReflection(), fragmentShader.GetShaderStage());

    ProcessPushConstants(vertexShader, fragmentShader);
    CreateDescriptorSetLayouts();
    CreatePipelineLayout();
}

VulkanMaterialLayout::~VulkanMaterialLayout()
{
    if (m_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(VulkanContext::Get().Device(), m_PipelineLayout, nullptr);
    }

	m_DescriptorSetLayouts.clear();
}

void VulkanMaterialLayout::CreateDescriptorSetLayouts()
{
    for (const auto& [set, descriptors] : m_ShaderDescriptorInfo.setDescriptors)
    {
        VulkanDescriptorSetLayout::Builder builder;
        for (const auto& descriptor : descriptors)
        {
            builder.AddDescriptor(descriptor.binding, descriptor.type, descriptor.stageFlags, descriptor.count);
        }
        m_DescriptorSetLayouts.push_back(builder.Build());
    }
}

void VulkanMaterialLayout::CreatePipelineLayout()
{
    std::vector<VkDescriptorSetLayout> setLayouts;
    setLayouts.reserve(m_DescriptorSetLayouts.size());
	for (const auto& layout : m_DescriptorSetLayouts)
    {
        setLayouts.push_back(layout->GetDescriptorSetLayout());
    }

    std::vector<VkPushConstantRange> pushConstantRanges;
    pushConstantRanges.reserve(m_PushConstantRanges.size());
	for (const auto& range : m_PushConstantRanges)
    {
        pushConstantRanges.push_back({ range.stageFlags, range.offset, range.size });
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

	VK_CHECK_RESULT(vkCreatePipelineLayout(VulkanContext::Get().Device(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));
	std::string debugName = m_DebugName + " Pipeline Layout";
	SetDebugUtilsObjectName(VulkanContext::Get().Device(), VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)m_PipelineLayout, debugName.c_str());
}

void VulkanMaterialLayout::ProcessPushConstants(VulkanShader& vertShader, VulkanShader& fragShader)
{
    auto addPushConstants = [this](const VulkanShaderReflection& reflection, VkShaderStageFlags stageFlags)
    {
        for (const auto& pc : reflection.GetPushConstantRanges())
        {
            m_PushConstantRanges.push_back({ pc.name, stageFlags, pc.offset, pc.size });
        }
    };

    addPushConstants(vertShader.GetReflection(), VK_SHADER_STAGE_VERTEX_BIT);
    addPushConstants(fragShader.GetReflection(), VK_SHADER_STAGE_FRAGMENT_BIT);

    // Sort and merge overlapping ranges
    std::sort(m_PushConstantRanges.begin(), m_PushConstantRanges.end(),
              [](const PushConstantRange& a, const PushConstantRange& b) { return a.offset < b.offset; });

    for (size_t i = 1; i < m_PushConstantRanges.size(); )
    {
        auto& prev = m_PushConstantRanges[i - 1];
        auto& curr = m_PushConstantRanges[i];

        if (prev.offset + prev.size > curr.offset)
        {
            // Merge overlapping or adjacent ranges
            prev.size = std::max(prev.size, curr.offset + curr.size - prev.offset);
            prev.stageFlags |= curr.stageFlags;
            prev.name += "+" + curr.name; // Combine names for debugging
            m_PushConstantRanges.erase(m_PushConstantRanges.begin() + i);
        }
        else
        {
            ++i;
        }
    }

	// Merge ranges with the same stage flags
	for (size_t i = 1; i < m_PushConstantRanges.size(); )
	{
		auto& prev = m_PushConstantRanges[i - 1];
		auto& curr = m_PushConstantRanges[i];

		if (prev.stageFlags == curr.stageFlags)
		{
			prev.size = curr.offset + curr.size - prev.offset;
			prev.name += "+" + curr.name;
			m_PushConstantRanges.erase(m_PushConstantRanges.begin() + i);
		}
		else
		{
			++i;
		}
	}
}
