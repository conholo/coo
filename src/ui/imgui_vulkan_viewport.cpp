#include "imgui_vulkan_viewport.h"
#include "vulkan/vulkan_image.h"
#include <imgui.h>
#include <vulkan/vulkan.h>


void VulkanImGuiViewport::Draw(VulkanImage2D& displayImage)
{
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

	auto& swapchain = Application::Get().GetRenderer().VulkanSwapchain();
	const auto textureID = ImGui_ImplVulkan_AddTexture(
		displayImage.GetSampler(),
		displayImage.GetView()->GetImageView(),
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	ImGui::Image(
		textureID,
		ImVec2{ m_ViewportSize.x, m_ViewportSize.y },
		ImVec2{ 0, 1 }, ImVec2{ 1, 0 }
 	);

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
