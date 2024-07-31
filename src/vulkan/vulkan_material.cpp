#include "vulkan_material.h"
#include "vulkan_descriptors.h"

void VulkanMaterial::CreateDescriptorSetLayout()
{
    VulkanDescriptorSetLayout::Builder builder{m_Device};

    for (const auto& binding : m_VertexShader->GetDescriptorSetLayoutBindings()) {
        builder.AddBinding(binding.binding, binding.descriptorType, binding.stageFlags, binding.descriptorCount);
    }
    for (const auto& binding : m_FragmentShader->GetDescriptorSetLayoutBindings()) {
        builder.AddBinding(binding.binding, binding.descriptorType, binding.stageFlags, binding.descriptorCount);
    }

    m_DescriptorSetLayout = builder.Build();
}

void VulkanMaterial::AllocateDescriptorSets()
{
    // Assuming you have a shared descriptor pool
    VulkanDescriptorPool::Builder poolBuilder;
    // Add pool sizes based on shader reflection data
    m_DescriptorPool = poolBuilder.Build();

    VkDescriptorSet descriptorSet;
    m_DescriptorPool->AllocateDescriptor(m_DescriptorSetLayout->GetDescriptorSetLayout(), descriptorSet);
    m_DescriptorSets.push_back(descriptorSet);
}