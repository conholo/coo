#pragma once

#include <glm/vec2.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

class RenderGraph;
struct FrameInfo;
class VulkanImage2D;
class VulkanDescriptorPool;
class VulkanDescriptorSetLayout;

class VulkanImGuiViewport
{
public:
	VulkanImGuiViewport() = default;
	void Initialize(RenderGraph& graph);
	void Draw(RenderGraph& graph, FrameInfo& frameInfo);
	bool ShouldBlockEvents() const { return !m_ViewportFocused && !m_ViewportHovered; }

private:
	void CalculateViewportSize(VulkanImage2D& displayImage);
private:
	glm::vec2		 m_ViewportSize{ 0.0f };
	glm::vec2		 m_ViewportBoundsMin{ 0.0f };
	glm::vec2		 m_ViewportBoundsMax{ 0.0f };
	bool			 m_ViewportFocused = false;
	bool			 m_ViewportHovered = false;

	std::vector<VkDescriptorSet> m_DescriptorSets;
	std::unique_ptr<VulkanDescriptorPool> m_DescriptorPool;
	std::unique_ptr<VulkanDescriptorSetLayout> m_SetLayout;
};
