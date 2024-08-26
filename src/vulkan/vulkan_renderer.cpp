#include <vulkan/render_passes/swapchain_pass.h>

#include "vulkan_renderer.h"

#include "core/frame_info.h"
#include "core/game_object.h"
#include "core/platform_path.h"
#include "render_passes/gbuffer_render_pass.h"
#include "render_passes/lighting_render_pass.h"
#include "render_passes/render_graph.h"
#include "render_passes/render_pass_resources/all_resources.h"
#include "render_passes/render_graph_resource_declarations.h"
#include "vulkan_utils.h"
#include <imgui.h>

VulkanRenderer::VulkanRenderer(Window& window, RenderGraph& graph)
	: m_WindowRef(window), m_GraphRef(graph)
{
}

void VulkanRenderer::Shutdown()
{
	m_SwapchainRenderer->Shutdown(m_GraphRef);
	m_SwapchainRenderer = nullptr;
}

void VulkanRenderer::Initialize()
{
	m_SwapchainRenderer = std::make_shared<VulkanSwapchainRenderer>(m_GraphRef, m_WindowRef);
	m_SwapchainRenderer->SetOnRecreateSwapchainCallback(
		[this](uint32_t width, uint32_t height)
		{
			m_CurrentFrameIndex = 0;
			OnSwapchainRecreate(width, height);
		});

	// Create Global Resources
	auto createGlobalUbos =
		[](size_t index, const std::string& resourceBaseName)
	{
		auto resourceName = resourceBaseName + " " + std::to_string(index);
		auto bufferResource = std::make_unique<BufferResource>(resourceName);
		bufferResource->Create(
			sizeof(GlobalUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		return bufferResource;
	};
	m_GraphRef.CreateResources<BufferResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		GlobalUniformBufferResourceName,
		createGlobalUbos);

	auto createShader =
		[](const std::string& resourceBaseName, const std::string& filePath, ShaderType shaderType)
	{
		auto shaderResource = std::make_unique<ShaderResource>(resourceBaseName);
		shaderResource->Create(filePath, shaderType);
		return shaderResource;
	};
	auto shaderDirectory = FileSystemUtil::GetShaderDirectory();
	auto fsqVertPath = FileSystemUtil::PathToString(shaderDirectory / "fsq.vert");
	m_GraphRef.CreateResource<ShaderResource>(
		FullScreenQuadShaderResourceName,
		createShader,
		fsqVertPath,
		ShaderType::Vertex);

	m_GraphRef.AddPass<GBufferPass>(
		{
			GlobalUniformBufferResourceName,
			SwapchainImageAvailableSemaphoreResourceName
		},
		{
			GBufferCommandBufferResourceName,
			GBufferGraphicsPipelineResourceName,
			GBufferRenderPassResourceName,
			GBufferVertexShaderResourceName,
			GBufferFragmentShaderResourceName,
			GBufferFramebufferResourceName,
			GBufferMaterialLayoutResourceName,
			GBufferMaterialResourceName,
			GBufferAlbedoAttachmentTextureResourceName,
			GBufferPositionAttachmentTextureResourceName,
			GBufferNormalAttachmentTextureResourceName,
			GBufferDepthAttachmentTextureResourceName,
			GBufferRenderCompleteSemaphoreResourceName,
			GBufferResourcesInFlightResourceName
		});
	m_GraphRef.AddPass<LightingPass>();
	m_GraphRef.AddPass<SceneCompositionPass>();
	m_GraphRef.AddPass<SwapchainPass>();

	m_GraphRef.Initialize();
}

void VulkanRenderer::OnEvent(Event& event)
{
}

void VulkanRenderer::Render(FrameInfo& frameInfo)
{
	bool success = m_SwapchainRenderer->BeginFrame(m_GraphRef, m_CurrentFrameIndex);
	{
		if (!success)
			return;

		GlobalUbo ubo{
			.Projection = frameInfo.Cam.GetProjection(),
			.View = frameInfo.Cam.GetView(),
			.InvView = frameInfo.Cam.GetInvView(),
			.InvProjection = frameInfo.Cam.GetInvProjection(),
			.CameraPosition = glm::vec4(frameInfo.Cam.GetPosition(), 1.0)};

		auto uboResource = m_GraphRef.GetResource<BufferResource>(GlobalUniformBufferResourceName, frameInfo.FrameIndex);
		uboResource->Get()->WriteToBuffer(&ubo);
		uboResource->Get()->Flush();
		frameInfo.ImageIndex = m_SwapchainRenderer->m_CurrentImageIndex;
		m_GraphRef.Execute(frameInfo);
	}

	m_SwapchainRenderer->EndFrame(m_GraphRef, m_CurrentFrameIndex);
	m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % VulkanSwapchain::MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::PrepareGameObjectForRendering(GameObject& gameObjectRef)
{
	auto objectMaterialResource = m_GraphRef.GetResource<MaterialResource>(GBufferMaterialResourceName);
	gameObjectRef.Material = objectMaterialResource->Get()->Clone();
}

void VulkanRenderer::OnSwapchainRecreate(uint32_t width, uint32_t height)
{
	m_GraphRef.OnSwapchainResize(width, height);
}
