#include <vulkan/render_passes/swapchain_render_pass.h>

#include "vulkan_renderer.h"

#include "core/frame_info.h"
#include "core/game_object.h"
#include "render_passes/gbuffer_render_pass.h"
#include "ui/imgui_vulkan_dock_space.h"
#include "vulkan_deferred_renderer.h"
#include "vulkan_utils.h"

#include <core/application.h>
#include <imgui.h>
#include <vulkan/render_passes/render_graph.h>

VulkanRenderer::VulkanRenderer(Window& window) : m_WindowRef(window)
{
	m_SwapchainRenderer = std::make_shared<VulkanSwapchainRenderer>(window);
	m_SwapchainRenderer->SetOnRecreateSwapchainCallback(
		[this](uint32_t width, uint32_t height)
		{
			m_CurrentFrameIndex = 0;
			OnSwapchainRecreate(width, height);
		});
	m_ImGuiRenderer = std::make_unique<VulkanImGuiRenderer>();
	m_ImGuiViewport = std::make_unique<VulkanImGuiViewport>();
	m_SceneRenderer = std::make_unique<VulkanDeferredRenderer>(this);
}

void VulkanRenderer::Shutdown()
{
	m_SwapchainRenderer->Shutdown();
	m_SwapchainRenderer = nullptr;
	m_GlobalUboBuffers.clear();
	m_SceneRenderer->Shutdown();
	m_ImGuiRenderer->Shutdown();
	m_SceneRenderer = nullptr;
	m_ImGuiRenderer = nullptr;
}

void VulkanRenderer::Initialize()
{
	for (auto& uboBuffer : m_GlobalUboBuffers)
	{
		uboBuffer = std::make_shared<VulkanBuffer>(
			sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		uboBuffer->Map();
	}

	m_SceneRenderer->Initialize();
	m_ImGuiRenderer->Initialize(m_SceneRenderer->GetRenderFinishedRenderPass());

	auto swapchainPass = std::unique_ptr<SwapchainPass>();

	RenderGraph graph;
	auto gBufferPass = std::make_unique<GBufferPass>();

	/*
	 * 	Render Graph Loop
	 *  1. Get next
	 *  2.
	 */
}

void VulkanRenderer::OnEvent(Event& event)
{
	m_SceneRenderer->OnEvent(event);
	m_ImGuiRenderer->OnEvent(event);
}

void VulkanRenderer::Render(FrameInfo& frameInfo)
{
	// Try to acquire the next image.
	std::shared_ptr<VulkanCommandBuffer> cmd = m_SwapchainRenderer->BeginFrame(m_CurrentFrameIndex).lock();
	{
		if (cmd == nullptr)
		{
			// Swapchain needs to be recreated, don't proceed with rendering
			std::cout << "Swapchain recreation needed, skipping render for this cycle" << std::endl;
			return;
		}

		GlobalUbo ubo{.Projection = frameInfo.Cam.GetProjection(),
			.View = frameInfo.Cam.GetView(),
			.InvView = frameInfo.Cam.GetInvView(),
			.InvProjection = frameInfo.Cam.GetInvProjection(),
			.CameraPosition = glm::vec4(frameInfo.Cam.GetPosition(), 1.0)};

		std::weak_ptr<VulkanBuffer> uboBuffer = m_GlobalUboBuffers[m_CurrentFrameIndex];
		auto sharedBuffer = uboBuffer.lock();

		frameInfo.GlobalUbo = sharedBuffer;
		sharedBuffer->WriteToBuffer(&ubo);
		sharedBuffer->Flush();
		frameInfo.SwapchainSubmitCommandBuffer = cmd;
		frameInfo.ImageIndex = m_SwapchainRenderer->m_CurrentImageIndex;
		frameInfo.RendererCompleteSemaphore = m_SceneRenderer->GetRendererFinishedSemaphore(frameInfo.FrameIndex);

		// Submit the scene rendering commands.
		m_SceneRenderer->Render(frameInfo);
		m_ImGuiRenderer->Begin();
		{
			ImGuiVulkanDockSpace::Begin();

			ImGui::Begin("Deferred Renderer Debug");
			{
				ImGui::Text("Frame Index: %d", frameInfo.FrameIndex);
				ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
				if (ImGui::Button("Toggle Wireframe"))
				{
				}
			}
			ImGui::End();

			// m_ImGuiRenderer->BlockEvents(m_ImGuiViewport->ShouldBlockEvents());

			ImGuiVulkanDockSpace::End();
		}

		m_ImGuiRenderer->RecordImGuiPass(frameInfo);
		m_ImGuiRenderer->End();
	}

	// Submit the swapchain draw commands and present.
	m_SwapchainRenderer->EndFrame(frameInfo);
	m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % VulkanSwapchain::MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::PrepareGameObjectForRendering(GameObject& gameObjectRef)
{
	m_SceneRenderer->RegisterGameObject(gameObjectRef);
}

void VulkanRenderer::OnSwapchainRecreate(uint32_t width, uint32_t height)
{
	m_SceneRenderer->Resize(width, height);
}
