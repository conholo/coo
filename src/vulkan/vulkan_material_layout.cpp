#include "vulkan_material_layout.h"
#include "vulkan_context.h"

VulkanMaterialLayout::VulkanMaterialLayout(const std::shared_ptr<VulkanShader>& vertexShader, const std::shared_ptr<VulkanShader>& fragmentShader)
        : m_VertexShader(vertexShader), m_FragmentShader(fragmentShader)
{
    m_ShaderDescriptorInfo.AddShaderReflection(m_VertexShader->GetReflection(), m_VertexShader->GetShaderStage());
    m_ShaderDescriptorInfo.AddShaderReflection(m_FragmentShader->GetReflection(), m_FragmentShader->GetShaderStage());

    ProcessPushConstants();
    CreateDescriptorSetLayouts();
    CreatePipelineLayout();
}

VulkanMaterialLayout::~VulkanMaterialLayout()
{
    if (m_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(VulkanContext::Get().Device(), m_PipelineLayout, nullptr);
    }

	if(m_VertexShader)
	{
		m_VertexShader.reset();
		m_VertexShader = nullptr;
	}

	if(m_FragmentShader)
	{
		m_FragmentShader.reset();
		m_FragmentShader = nullptr;
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
    for (const auto& layout : m_DescriptorSetLayouts)
    {
        setLayouts.push_back(layout->GetDescriptorSetLayout());
    }

    std::vector<VkPushConstantRange> pushConstantRanges;
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

    if (vkCreatePipelineLayout(VulkanContext::Get().Device(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
}

void VulkanMaterialLayout::ProcessPushConstants()
{
    auto addPushConstants = [this](const VulkanShaderReflection& reflection, VkShaderStageFlags stageFlags)
    {
        for (const auto& pc : reflection.GetPushConstantRanges())
        {
            m_PushConstantRanges.push_back({ pc.name, stageFlags, pc.offset, pc.size });
        }
    };

    addPushConstants(m_VertexShader->GetReflection(), VK_SHADER_STAGE_VERTEX_BIT);
    addPushConstants(m_FragmentShader->GetReflection(), VK_SHADER_STAGE_FRAGMENT_BIT);

    // Sort and merge overlapping ranges
    std::sort(m_PushConstantRanges.begin(), m_PushConstantRanges.end(),
              [](const PushConstantRange& a, const PushConstantRange& b) { return a.offset < b.offset; });

    for (size_t i = 1; i < m_PushConstantRanges.size(); /* no increment */)
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
}
