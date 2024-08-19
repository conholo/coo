#include "vulkan_renderer.h"

#include "core/frame_info.h"
#include "core/game_object.h"
#include "render_passes/gbuffer_render_pass.h"
#include "ui/imgui_vulkan_dock_space.h"
#include "vulkan_deferred_renderer.h"
#include "vulkan_utils.h"
#include "vulkan/render_passes/lighting_render_pass.h"
#include "vulkan/render_passes/render_graph.h"
#include "render_passes/scene_composition_pass.h"

#include <core/application.h>
#include <core/platform_path.h>
#include <imgui.h>

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

	RenderGraph graph;

	// Create Global Resources
	auto createGlobalUbos =
		[](size_t index, const std::string& resourceBaseName)
	{
		auto resourceName = resourceBaseName + " " + std::to_string(index);
		std::shared_ptr<VulkanBuffer> uboBuffer = std::make_shared<VulkanBuffer>(
			sizeof(GlobalUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

		return std::make_shared<BufferResource>(resourceName, uboBuffer);
	};
	graph.CreateResources<BufferResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, "Global Uniform Buffer", createGlobalUbos);

	auto createShader =
		[](const std::string& resourceBaseName, const std::string& filePath, ShaderType shaderType)
	{
		auto shader = std::make_shared<VulkanShader>(filePath, shaderType);
		return std::make_shared<ShaderResource>(resourceBaseName, shader);
	};
	auto shaderDirectory = FileSystemUtil::GetShaderDirectory();
	auto fsqVertPath = FileSystemUtil::PathToString(shaderDirectory / "fsq.vert");
	graph.CreateResource<ShaderResource>("Full Screen Quad Vertex Shader", createShader, fsqVertPath, ShaderType::Vertex);

	graph.AddPass<GBufferPass>();
	graph.AddPass<LightingPass>();
	graph.AddPass<SceneCompositionPass>();
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
