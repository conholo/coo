#include <vulkan/render_passes/swapchain_pass.h>

#include "vulkan_renderer.h"

#include "core/frame_info.h"
#include "core/game_object.h"
#include "core/platform_path.h"
#include "render_passes/gbuffer_render_pass.h"
#include "render_passes/lighting_render_pass.h"
#include "render_passes/render_graph.h"
#include "render_passes/render_graph_resource_declarations.h"
#include "vulkan_utils.h"

#include <imgui.h>

VulkanRenderer::VulkanRenderer(Window& window) : m_WindowRef(window)
{
	m_SwapchainRenderer = std::make_shared<VulkanSwapchainRenderer>(m_Graph, window);
	m_SwapchainRenderer->SetOnRecreateSwapchainCallback(
		[this](uint32_t width, uint32_t height)
		{
			m_CurrentFrameIndex = 0;
			OnSwapchainRecreate(width, height);
		});
}

void VulkanRenderer::Shutdown()
{
	m_SwapchainRenderer->Shutdown(m_Graph);
	m_SwapchainRenderer = nullptr;
}

void VulkanRenderer::Initialize()
{
	m_ImGuiRenderer = std::make_shared<VulkanImGuiRenderer>();
	m_ImGuiRenderer->Initialize(m_Graph);
	m_ImGuiViewport = std::make_unique<VulkanImGuiViewport>();

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
		uboBuffer->Map();
		return std::make_shared<BufferResource>(resourceName, uboBuffer);
	};
	m_Graph.CreateResources<BufferResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, GlobalUniformBufferResourceName, createGlobalUbos);

	auto createShader =
		[](const std::string& resourceBaseName, const std::string& filePath, ShaderType shaderType)
	{
		auto shader = std::make_shared<VulkanShader>(filePath, shaderType);
		return std::make_shared<ShaderResource>(resourceBaseName, shader);
	};
	auto shaderDirectory = FileSystemUtil::GetShaderDirectory();
	auto fsqVertPath = FileSystemUtil::PathToString(shaderDirectory / "fsq.vert");
	m_Graph.CreateResource<ShaderResource>(FullScreenQuadShaderResourceName, createShader, fsqVertPath, ShaderType::Vertex);

	m_Graph.AddPass<GBufferPass>();
	m_Graph.AddPass<LightingPass>();
	m_Graph.AddPass<SwapchainPass>();

	m_Graph.Initialize();
}

void VulkanRenderer::OnEvent(Event& event)
{
}

void VulkanRenderer::Render(FrameInfo& frameInfo)
{
	bool success = m_SwapchainRenderer->BeginFrame(m_Graph, m_CurrentFrameIndex);
	{
		if (!success)
			return;

		GlobalUbo ubo{
			.Projection = frameInfo.Cam.GetProjection(),
			.View = frameInfo.Cam.GetView(),
			.InvView = frameInfo.Cam.GetInvView(),
			.InvProjection = frameInfo.Cam.GetInvProjection(),
			.CameraPosition = glm::vec4(frameInfo.Cam.GetPosition(), 1.0)};

		auto uboResource = m_Graph.GetResource<BufferResource>(GlobalUniformBufferResourceName, frameInfo.FrameIndex);
		uboResource->Get()->WriteToBuffer(&ubo);
		uboResource->Get()->Flush();
		frameInfo.ImageIndex = m_SwapchainRenderer->m_CurrentImageIndex;

		m_ImGuiRenderer->Begin();
		m_ImGuiRenderer->Update(m_Graph, frameInfo.FrameIndex);
		m_ImGuiRenderer->End();

		m_Graph.Execute(frameInfo);
	}

	m_SwapchainRenderer->EndFrame(m_Graph, m_CurrentFrameIndex);
	m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % VulkanSwapchain::MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::PrepareGameObjectForRendering(GameObject& gameObjectRef)
{
	auto objectMaterialResource = m_Graph.GetResource<MaterialResource>(GBufferMaterialResourceName);
	gameObjectRef.Material = objectMaterialResource->Get()->Clone();
}

void VulkanRenderer::OnSwapchainRecreate(uint32_t width, uint32_t height)
{
	m_Graph.OnSwapchainResize(width, height);
}
