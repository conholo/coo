#include "vulkan_renderer.h"

#include "core/frame_info.h"
#include "vulkan_deferred_renderer.h"
#include "vulkan_simple_renderer.h"
#include "vulkan_utils.h"

#include <memory>

VulkanRenderer::VulkanRenderer(Window& window) : m_WindowRef(window)
{
	m_SwapchainRenderer = std::make_unique<VulkanSwapchainRenderer>(window);
	m_SwapchainRenderer->SetOnRecreateSwapchainCallback(
		[this](uint32_t width, uint32_t height)
		{
			m_CurrentFrameIndex = 0;
			OnSwapchainRecreate(width, height);
		});
	m_Renderer = std::make_unique<VulkanDeferredRenderer>(this);
}

VulkanRenderer::~VulkanRenderer()
{
	Shutdown();
}

void VulkanRenderer::Initialize()
{
	for (auto& uboBuffer : m_GlobalUboBuffers)
	{
		uboBuffer = std::make_shared<VulkanBuffer>(
			sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		uboBuffer->Map();
	}

	m_Renderer->Initialize();
}

void VulkanRenderer::Render(FrameInfo& frameInfo)
{
	VkCommandBuffer cmd = m_SwapchainRenderer->BeginFrame(m_CurrentFrameIndex);
	if (cmd == VK_NULL_HANDLE)
	{
		// Swapchain needs to be recreated, don't proceed with rendering
		std::cout << "Swapchain recreation needed, skipping render for this cycle" << std::endl;
		return;
	}

	GlobalUbo ubo{
		.Projection = frameInfo.Cam.GetProjection(),
		.View = frameInfo.Cam.GetView(),
		.InvView = frameInfo.Cam.GetInvView(),
		.InvProjection = frameInfo.Cam.GetInvProjection(),
		.CameraPosition = glm::vec4(frameInfo.Cam.GetPosition(), 1.0)};

	frameInfo.GlobalUbo = m_GlobalUboBuffers[m_CurrentFrameIndex];
	frameInfo.GlobalUbo->WriteToBuffer(&ubo);
	frameInfo.GlobalUbo->Flush();
	frameInfo.DrawCommandBuffer = cmd;

	m_Renderer->Render(frameInfo);

	m_SwapchainRenderer->EndFrame(frameInfo);
	m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % VulkanSwapchain::MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::PrepareGameObjectForRendering(GameObject& gameObjectRef)
{
	m_Renderer->RegisterGameObject(gameObjectRef);
}

void VulkanRenderer::OnSwapchainRecreate(uint32_t width, uint32_t height)
{
	m_Renderer->Resize(width, height);
}

void VulkanRenderer::Shutdown()
{
}
