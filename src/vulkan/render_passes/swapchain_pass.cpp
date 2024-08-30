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
	VulkanCommandBuffer& cmd = m_CommandBufferHandles[frameInfo.FrameIndex]->Get();
	VulkanFramebuffer& fbo = m_FramebufferHandles[frameInfo.ImageIndex]->Get();
	VulkanRenderPass& renderPass = m_RenderPassHandle->Get();
	VulkanFence& fence = m_ResourcesInFlightFenceHandles[frameInfo.FrameIndex]->Get();

	cmd.WaitForCompletion(fence.GetHandle());
	cmd.Begin();
	{
		VkRenderPassBeginInfo compositionRenderPassInfo{};
		compositionRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		compositionRenderPassInfo.renderPass = renderPass.GetHandle();
		compositionRenderPassInfo.framebuffer = fbo.GetHandle();
		compositionRenderPassInfo.renderArea.offset = {0, 0};
		auto attachmentExtent = VkExtent2D{ fbo.Width(), fbo.Height() };
		compositionRenderPassInfo.renderArea.extent = attachmentExtent;

		renderPass.BeginPass(cmd.GetHandle(), compositionRenderPassInfo, attachmentExtent);
		{
			m_UIPass.Record(frameInfo, graph);
		}
		renderPass.EndPass(cmd.GetHandle());
	}
	cmd.End();
}

void SwapchainPass::Submit(const FrameInfo& frameInfo, RenderGraph& graph)
{
	// Submit the command buffer with all subpasses
	VulkanCommandBuffer& cmd = m_CommandBufferHandles[frameInfo.FrameIndex]->Get();
	VulkanFence& resourceFence = m_ResourcesInFlightFenceHandles[frameInfo.FrameIndex]->Get();
	ResourceHandle<FenceResource> imageInFlightFenceHandle = m_ImagesInFlightHandles[frameInfo.ImageIndex];

	// Make sure the active image is available (i.e. the last command to work on the swapchain image has finished)
	if(imageInFlightFenceHandle.Get() != nullptr)
	{
		std::vector<VkFence> fences = {imageInFlightFenceHandle->Get().GetHandle() };
		vkWaitForFences(VulkanContext::Get().Device(), 1, fences.data(), VK_TRUE, std::numeric_limits<int>::max());
	}
	// Assign the new resourceFence for the active image to the resource resourceFence for this frame in flight.
	imageInFlightFenceHandle->Set(&resourceFence);

	VulkanSemaphore& swapchainRenderComplete = m_RenderCompleteSemaphoreHandles[frameInfo.FrameIndex]->Get();
	VulkanSemaphore& sceneCompRenderComplete = graph.GetGlobalResourceHandle<SemaphoreResource>(SceneCompositionRenderCompleteSemaphoreResourceName, frameInfo.FrameIndex)->Get();
	std::vector<VkSemaphore> waitSemaphores = { sceneCompRenderComplete.GetHandle() };
	std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkSemaphore> signalSemaphores = { swapchainRenderComplete.GetHandle() };
	std::vector<VulkanCommandBuffer*> cmdBuffers = { &cmd };

	ResourceHandle<TextureResource> sceneCompTexResourceHandle = graph.GetResourceHandle<TextureResource>(SceneCompositionColorAttachmentResourceName, m_uuid, frameInfo.FrameIndex);
	sceneCompTexResourceHandle->Get().TransitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	VulkanCommandBuffer::Submit(
		VulkanContext::Get().GraphicsQueue(),
		cmdBuffers,
		waitSemaphores,
		waitStages,
		signalSemaphores,
		resourceFence.GetHandle());

	auto swapchainImage = graph.GetGlobalResourceHandle<Image2DResource>(SwapchainImage2DResourceName, frameInfo.ImageIndex)->Get();
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
		[&extent, this](
			size_t index,
			const std::string& name,
			VulkanRenderPass* renderPass,
			const std::string& swapchainImage2DResourceName)
	{
		auto image2DResource = graph.GetResource<Image2DResource>(swapchainImage2DResourceName, index);

		std::vector<VkImageView> attachments = {
			image2DResource->Get()->GetView()->GetImageView()
		};

		auto framebuffer = std::make_shared<VulkanFramebuffer>(resourceName);
		framebuffer->Create(renderPass->GetHandle(), attachments, extent.width, extent.height);
		return std::make_unique<FramebufferResource>(resourceBaseName, framebuffer);
	};

	VulkanRenderPass& renderPass = graph.GetResource(m_RenderPassHandle);

	m_FramebufferHandles = graph.CreateResources<FramebufferResource>(
		swapchain.ImageCount(),
		SwapchainFramebufferResourceName,
		createFramebuffer,
		renderPassResource->Get().get(),
		SwapchainImage2DResourceName);
}

void SwapchainPass::OnSwapchainResize(uint32_t width, uint32_t height, RenderGraph& graph)
{
	for (auto cmdBufferHandle : m_CommandBufferHandles)
	{
		auto cmdResource = graph.GetResource(cmdBufferHandle);

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(VulkanContext::Get().Device(), &fenceCreateInfo, nullptr, &fence));
		VK_CHECK_RESULT(cmdResource->Get()->InterruptAndReset(fence, true));
		vkDestroyFence(VulkanContext::Get().Device(), fence, nullptr);
	}

	auto freeFences = [](const std::shared_ptr<VulkanFence>& fence){};
	graph.TryFreeResources<FenceResource>(SwapchainResourcesInFlightFenceResourceName, freeFences);
	auto freeSemaphores = [](const std::shared_ptr<VulkanSemaphore>& semaphore){};
	graph.TryFreeResources<SemaphoreResource>(SwapchainRenderingCompleteSemaphoreResourceName, freeSemaphores);
	CreateSynchronizationPrimitives(graph);

	graph.TryFreeResources<RenderPassObjectResource>(SwapchainRenderPassResourceName,  [](const std::shared_ptr<VulkanRenderPass>& renderPass){});
	CreateRenderPass(graph);

	graph.TryFreeResources<FramebufferResource>(SwapchainFramebufferResourceName,  [](const std::shared_ptr<VulkanFramebuffer>& framebuffer){});
	CreateFramebuffers(graph);

	m_UIPass.OnSwapchainResize(width, height, graph);
}
