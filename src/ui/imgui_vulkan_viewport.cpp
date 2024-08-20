#include "imgui_vulkan_viewport.h"

#include "vulkan/vulkan_image.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>

void VulkanImGuiViewport::Initialize()
{
	VulkanDescriptorPool::Builder poolBuilder;
	poolBuilder
		.AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VulkanSwapchain::MAX_FRAMES_IN_FLIGHT)
		.SetMaxSets(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	m_DescriptorPool = poolBuilder.Build();

	m_DescriptorSets.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);

	VulkanDescriptorSetLayout::Builder builder;
	builder.AddDescriptor(
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		1);
	m_SetLayout = builder.Build();

	for(int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VulkanDescriptorWriter(*m_SetLayout, *m_DescriptorPool)
			.Build(m_DescriptorSets[i]);
	}
}

void VulkanImGuiViewport::Draw(RenderGraph& graph, FrameInfo& frameInfo)
{
	auto& displayImage = *graph.GetResource<Image2DResource>(SwapchainImage2DResourceName, frameInfo.ImageIndex)->Get();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
	ImGui::Begin("Viewport");
	CalculateViewportSize(displayImage);

	ImVec2 availContentSize = ImGui::GetContentRegionAvail();
	ImVec2 cursorPos;
	cursorPos.x = (availContentSize.x - m_ViewportSize.x) * 0.5f;
	cursorPos.y = (availContentSize.y - m_ViewportSize.y) * 0.5f;

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	ImVec2 rectMin = ImGui::GetCursorScreenPos();
	ImVec2 rectMax = ImVec2(rectMin.x + availContentSize.x, rectMin.y + availContentSize.y);
	drawList->AddRectFilled(rectMin, rectMax, IM_COL32(0, 0, 0, 255));

	ImGui::SetCursorPos(cursorPos);

	displayImage.TransitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	m_DescriptorPool->ResetPool();
	VulkanDescriptorWriter(*m_SetLayout, *m_DescriptorPool)
		.WriteImage(0, displayImage.GetDescriptorInfo())
		.Build(m_DescriptorSets[frameInfo.FrameIndex]);

	const auto textureID = m_DescriptorSets[frameInfo.FrameIndex];

	ImGui::Image(
		textureID,
		ImVec2{ m_ViewportSize.x, m_ViewportSize.y },
		ImVec2{ 0, 1 }, ImVec2{ 1, 0 }
 	);
	displayImage.TransitionLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	ImGui::End();
	ImGui::PopStyleVar();
}

void VulkanImGuiViewport::CalculateViewportSize(VulkanImage2D& displayImage)
{
	auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
	auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
	auto viewportOffset = ImGui::GetWindowPos();
	m_ViewportBoundsMin = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
	m_ViewportBoundsMax = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

	m_ViewportFocused = ImGui::IsWindowFocused();
	m_ViewportHovered = ImGui::IsWindowHovered();

	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

	// Calculate aspect ratio of the original image
	float aspectRatio = static_cast<float>(displayImage.GetSpecification().Width) / static_cast<float>(displayImage.GetSpecification().Height);
	float viewportWidth = viewportPanelSize.x;
	float viewportHeight = viewportPanelSize.x / aspectRatio;

	if(viewportHeight > viewportPanelSize.y)
	{
		viewportHeight = viewportPanelSize.y;
		viewportWidth = viewportPanelSize.y * aspectRatio;
	}

	m_ViewportSize = { viewportWidth, viewportHeight };
}
