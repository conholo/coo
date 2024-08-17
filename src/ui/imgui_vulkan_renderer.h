#pragma once

#include <memory>
#include <vector>

#include "core/event/event.h"
#include <vulkan/vulkan.h>

class VulkanRenderPass;
class VulkanFramebuffer;
class VulkanGraphicsPipeline;
class VulkanSwapchainRenderer;
class Window;

class VulkanImGuiRenderer
{
public:
	VulkanImGuiRenderer() = default;
	~VulkanImGuiRenderer() = default;

	void Initialize(VulkanRenderPass& renderPassRef);
	void Shutdown();

	void Begin();
	void RecordImGuiPass(FrameInfo& frameInfo);
	void End();

	void OnEvent(Event& e) const;
	void BlockEvents(bool block) { m_BlockEvents = block; }

private:
	void InitializeFontTexture();
	void UpdateBuffers();
	static void SetDarkThemeColors();

private:
	struct DisplayTransformPushConstants
	{
		glm::vec2 Scale{};
		glm::vec2 Translate{};
	} m_TransformPushConstants{};

	std::vector<VkSemaphore> m_RenderCompleteSemaphores;
	std::vector<std::unique_ptr<VulkanFramebuffer>> m_ImGuiFramebuffers;

	std::shared_ptr<VulkanShader> m_UIVertexShader;
	std::shared_ptr<VulkanShader> m_UIFragmentShader;
	std::shared_ptr<VulkanMaterialLayout> m_UIMaterialLayout;
	std::unique_ptr<VulkanMaterial> m_UIMaterial;
	std::unique_ptr<VulkanGraphicsPipeline> m_UIPipeline;

	std::unique_ptr<VulkanImage2D> m_FontImage;
	std::unique_ptr<VulkanBuffer> m_VertexBuffer;
	size_t m_VertexCount{};
	std::unique_ptr<VulkanBuffer> m_IndexBuffer;
	size_t m_IndexCount{};

	bool m_BlockEvents = true;
};