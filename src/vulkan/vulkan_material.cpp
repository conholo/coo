#include "vulkan_material.h"
#include "vulkan_context.h"
#include "vulkan_swapchain.h"

VulkanMaterial::VulkanMaterial(VulkanMaterialLayout& layoutRef)
        : m_LayoutRef(layoutRef)
{
    AllocateDescriptorSets();
}

VulkanMaterial::~VulkanMaterial()
{
    // Descriptor sets are automatically freed when the pool is destroyed
}

VulkanMaterial::VulkanMaterial(VulkanMaterial&& other) noexcept
        : m_LayoutRef(other.m_LayoutRef),
          m_DescriptorPool(std::move(other.m_DescriptorPool)),
          m_DescriptorSets(std::move(other.m_DescriptorSets)),
          m_PushConstantData(std::move(other.m_PushConstantData))
{
}

VulkanMaterial& VulkanMaterial::operator=(VulkanMaterial&& other) noexcept
{
    if (this != &other)
    {
		m_LayoutRef = other.m_LayoutRef;
        m_DescriptorPool = std::move(other.m_DescriptorPool);
        m_DescriptorSets = std::move(other.m_DescriptorSets);
        m_PushConstantData = std::move(other.m_PushConstantData);
    }
    return *this;
}

void VulkanMaterial::AllocateDescriptorSets()
{
    VulkanDescriptorPool::Builder poolBuilder;
    for (const auto& [type, count] : m_LayoutRef.GetShaderDescriptorInfo().totalDescriptorCounts)
    {
		if(count == 0) continue;
        poolBuilder.AddPoolSize(type, count * VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
    }

	if(!poolBuilder.IsNotEmpty())
	{
		std::cout << "No types specified for descriptor pool - aborting descriptor set allocation" << "\n";
		return;
	}

    poolBuilder.SetMaxSets(m_LayoutRef.GetShaderDescriptorInfo().GetTotalUniqueSetCount() * VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
    m_DescriptorPool = poolBuilder.Build();

    m_DescriptorSets.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
    const auto& layouts = m_LayoutRef.GetDescriptorSetLayouts();

    for(int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
    {
        m_DescriptorSets[i].resize(layouts.size());
        for (size_t j = 0; j < layouts.size(); ++j)
        {
            VulkanDescriptorWriter(*layouts[j], *m_DescriptorPool)
                    .Build(m_DescriptorSets[i][j]);
        }
    }
}

void VulkanMaterial::BindDescriptors(uint32_t frameIndex, VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint)
{
    vkCmdBindDescriptorSets(
            commandBuffer,
            pipelineBindPoint, m_LayoutRef.GetPipelineLayout(),
            0,
            static_cast<uint32_t>(m_DescriptorSets[frameIndex].size()),
            m_DescriptorSets[frameIndex].data(),
            0,
            nullptr
    );
}

void VulkanMaterial::UpdateDescriptor(uint32_t frameIndex, uint32_t set, const DescriptorUpdate& update)
{
    UpdateDescriptorSets(frameIndex, {{set, {update}}});
}

void VulkanMaterial::UpdateDescriptorSets(uint32_t frameIndex, const std::vector<std::pair<uint32_t, std::vector<DescriptorUpdate>>>& updates)
{
    for (const auto& [set, descriptorUpdates] : updates)
    {
        if (set >= m_DescriptorSets.size())
        {
            throw std::runtime_error("Descriptor set index out of range");
        }

        VulkanDescriptorWriter writer(*m_LayoutRef.GetDescriptorSetLayouts()[set], *m_DescriptorPool);

        for (const auto& update : descriptorUpdates)
        {
            switch (update.type)
            {
                case DescriptorUpdate::Type::Buffer:
                    writer.WriteBuffer(update.binding, update.bufferInfo);
                    break;
                case DescriptorUpdate::Type::Image:
                    writer.WriteImage(update.binding, update.imageInfo);
                    break;
            }
        }

        writer.Overwrite(m_DescriptorSets[frameIndex][set]);
    }
}

void VulkanMaterial::BindPushConstants(VkCommandBuffer commandBuffer)
{
    const auto& pushConstantRanges = m_LayoutRef.GetPushConstantRanges();
    for (const auto& range : pushConstantRanges)
    {
        auto it = m_PushConstantData.find(range.name);
        if (it != m_PushConstantData.end())
        {
            vkCmdPushConstants(
                    commandBuffer, m_LayoutRef.GetPipelineLayout(),
                    range.stageFlags,
                    range.offset,
                    range.size,
                    it->second.data()
            );
        }
    }
}
std::shared_ptr<VulkanMaterial> VulkanMaterial::Clone() const
{
    return std::make_shared<VulkanMaterial>(m_LayoutRef);
}