#include "swapchain_pass.h"
#include "render_graph.h"
#include "core/application.h"
#include "render_graph_resource_declarations.h"
#include "core/frame_info.h"
#include "vulkan/vulkan_fence.h"
#include "vulkan/vulkan_semaphore.h"
#include "vulkan/vulkan_render_pass.h"
#include "vulkan/vulkan_framebuffer.h"

void SwapchainPass::CreateResources(RenderGraph& graph)
{
	CreateCommandBuffers(graph);
	CreateSynchronizationPrimitives(graph);
	CreateRenderPass(graph);
	CreateFramebuffers(graph);
	m_UIPass.CreateResources(graph);
}

void SwapchainPass::Record(const FrameInfo& frameInfo, RenderGraph& graph)
{
	auto cmd = graph.GetResource<CommandBufferResource>(m_CommandBufferHandles[frameInfo.FrameIndex])->Get();
	auto* fboResource = graph.GetResource<FramebufferResource>(m_FramebufferHandles[frameInfo.ImageIndex]);
	auto* rpResource = graph.GetResource<RenderPassObjectResource>(m_RenderPassHandle);
	auto* resourceFence = graph.GetResource<FenceResource>(m_ResourcesInFlightFenceHandles[frameInfo.FrameIndex]);

	cmd->WaitForCompletion(resourceFence->Get()->GetHandle());

	auto fbo = fboResource->Get();
	auto renderPass = rpResource->Get();

	cmd->Begin();
	{
		VkRenderPassBeginInfo compositionRenderPassInfo{};
		compositionRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		compositionRenderPassInfo.renderPass = renderPass->GetHandle();
		compositionRenderPassInfo.framebuffer = fbo->GetHandle();
		compositionRenderPassInfo.renderArea.offset = {0, 0};
		auto attachmentExtent = VkExtent2D{ fbo->Width(), fbo->Height() };
		compositionRenderPassInfo.renderArea.extent = attachmentExtent;

		renderPass->BeginPass(cmd->GetHandle(), compositionRenderPassInfo, attachmentExtent);
		{
			m_UIPass.Record(frameInfo, graph);
		}
		renderPass->EndPass(cmd->GetHandle());
	}
	cmd->End();
}

void SwapchainPass::Submit(const FrameInfo& frameInfo, RenderGraph& graph)
{
	// Submit the command buffer with all subpasses
	auto cmdBufferResource = graph.GetResource<CommandBufferResource>(m_CommandBufferHandles[frameInfo.FrameIndex]);
	auto resourceFence = graph.GetResource<FenceResource>(m_ResourcesInFlightFenceHandles[frameInfo.FrameIndex]);
	auto imageFence = graph.GetResource<FenceResource>(m_ImagesInFlightHandles[frameInfo.ImageIndex]);

	// Make sure the active image is available (i.e. the last command to work on the swapchain image has finished)
	if(imageFence->Get() != nullptr)
	{
		std::vector<VkFence> fences = {imageFence->Get()->GetHandle()};
		vkWaitForFences(VulkanContext::Get().Device(), 1, fences.data(), VK_TRUE, std::numeric_limits<int>::max());
	}
	// Assign the new fence for the active image to the resource fence for this frame in flight.
	imageFence->Set(resourceFence->Get());

	auto swapchainRenderComplete = graph.GetResource<SemaphoreResource>(m_RenderCompleteSemaphoreHandles[frameInfo.FrameIndex]);
	auto sceneCompRenderComplete = graph.GetResource<SemaphoreResource>(SceneCompositionRenderCompleteSemaphoreResourceName, frameInfo.FrameIndex);
	std::vector<VkSemaphore> waitSemaphores = {sceneCompRenderComplete->Get()->GetHandle() };
	std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkSemaphore> signalSemaphores = {swapchainRenderComplete->Get()->GetHandle() };
	std::vector<VulkanCommandBuffer*> cmdBuffers = { &*cmdBufferResource->Get() };

	auto sceneCompTexResource = graph.GetResource<TextureResource>(SceneCompositionColorAttachmentResourceName, frameInfo.FrameIndex);
	sceneCompTexResource->Get()->TransitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	VulkanCommandBuffer::Submit(
		VulkanContext::Get().GraphicsQueue(),
		cmdBuffers,
		waitSemaphores,
		waitStages,
		signalSemaphores,
		resourceFence->Get()->GetHandle());
	sceneCompTexResource->Get()->TransitionLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto swapchainImage = graph.GetResource<Image2DResource>(SwapchainImage2DResourceName, frameInfo.ImageIndex)->Get();
	swapchainImage->SetExpectedLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void SwapchainPass::CreateCommandBuffers(RenderGraph& graph)
{
	auto& context = VulkanContext::Get();

	auto createCommandBuffer =
		[](size_t index, const std::string& resourceBaseName, VkCommandPool pool, bool isPrimary)
	{
		auto resourceName = resourceBaseName + " " + std::to_string(index);
		auto commandBuffer = std::make_shared<VulkanCommandBuffer>(pool, isPrimary, resourceName);
		return std::make_shared<CommandBufferResource>(resourceName, commandBuffer);
	};

	m_CommandBufferHandles = graph.CreateResources<CommandBufferResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		SwapchainCommandBufferResourceName,
		createCommandBuffer,
		context.GraphicsCommandPool(),
		true);
}

void SwapchainPass::CreateSynchronizationPrimitives(RenderGraph& graph)
{
	auto createSemaphore = [](size_t index, const std::string& baseName)
	{
		auto resourceName = baseName + " " + std::to_string(index);
		auto semaphore = std::make_shared<VulkanSemaphore>(resourceName);
		return std::make_shared<SemaphoreResource>(resourceName, semaphore);
	};
	m_RenderCompleteSemaphoreHandles = graph.CreateResources<SemaphoreResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		SwapchainRenderingCompleteSemaphoreResourceName,
		createSemaphore);
	m_ImageAvailableSemaphoreHandles = graph.CreateResources<SemaphoreResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		SwapchainImageAvailableSemaphoreResourceName,
		createSemaphore);

	auto createFence = [](size_t index, const std::string& baseName, bool signaled, bool nullInitialize)
	{
		auto resourceName = baseName + " " + std::to_string(index);
		if(!nullInitialize)
		{
			auto fence = std::make_shared<VulkanFence>(resourceName, signaled);
			return std::make_shared<FenceResource>(resourceName, fence);
		}
		return std::make_shared<FenceResource>(resourceName);
	};
	auto imageCount = Application::Get().GetRenderer().VulkanSwapchain().ImageCount();

	// Null initialized
	m_ImagesInFlightHandles = graph.CreateResources<FenceResource>(
		imageCount,
		SwapchainImagesInFlightFenceResourceName,
		createFence,
		false,
		true);

	// Start signaled
	m_ResourcesInFlightFenceHandles = graph.CreateResources<FenceResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		SwapchainResourcesInFlightFenceResourceName,
		createFence,
		true,
		false);
}

void SwapchainPass::CreateRenderPass(RenderGraph& graph)
{
	auto& swapchain = Application::Get().GetRenderer().VulkanSwapchain();
	auto imageFormat = swapchain.SwapchainImageFormat();

	auto createRenderPass =
		[imageFormat](const std::string& resourceBaseName)
	{
		auto renderPass = std::make_shared<VulkanRenderPass>(resourceBaseName);

		// Final color attachment (swapchain image)
		renderPass->AddAttachment(
			{
				.Type = AttachmentType::Color,
				.Format = ImageUtils::VulkanFormatToImageFormat(imageFormat),
				.Samples = VK_SAMPLE_COUNT_1_BIT,
				.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
				.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.ClearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
			});

		SubpassDescription uiSubpass;
		uiSubpass.ColorAttachments = {0};
		renderPass->AddSubpass(uiSubpass);

		renderPass->AddDependency(
			VK_SUBPASS_EXTERNAL, 0,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_DEPENDENCY_BY_REGION_BIT);

		renderPass->AddDependency(
			0, VK_SUBPASS_EXTERNAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
			VK_DEPENDENCY_BY_REGION_BIT);

		renderPass->Build();
		return std::make_shared<RenderPassObjectResource>(resourceBaseName, renderPass);
	};
	m_RenderPassHandle = graph.CreateResource<RenderPassObjectResource>(SwapchainRenderPassResourceName, createRenderPass);
}

void SwapchainPass::CreateFramebuffers(RenderGraph& graph)
{
	auto& swapchain = Application::Get().GetRenderer().VulkanSwapchain();
	auto extent = swapchain.Extent();

	auto createFramebuffer =
		[&extent, &graph](
			size_t index,
			const std::string& resourceBaseName,
			VulkanRenderPass* renderPass,
			const std::string& swapchainImage2DResourceName)
	{
		auto image2DResource = graph.GetResource<Image2DResource>(swapchainImage2DResourceName, index);

		std::vector<VkImageView> attachments = {
			image2DResource->Get()->GetView()->GetImageView()
		};

		auto resourceName = resourceBaseName + " " + std::to_string(index);
		auto framebuffer = std::make_shared<VulkanFramebuffer>(resourceName);
		framebuffer->Create(renderPass->GetHandle(), attachments, extent.width, extent.height);
		return std::make_shared<FramebufferResource>(resourceBaseName, framebuffer);
	};

	auto renderPassResource = graph.GetResource(m_RenderPassHandle);

	m_FramebufferHandles = graph.CreateResources<FramebufferResource>(
		swapchain.ImageCount(),
		SwapchainFramebufferResourceName,
		createFramebuffer,
		renderPassResource->Get().get(),
		SwapchainImage2DResourceName);
}
