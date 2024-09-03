#include "gbuffer_render_pass.h"

#include "core/application.h"
#include "render_graph.h"
#include "render_graph_resource_declarations.h"
#include "vulkan/render_passes/render_pass_resources/texture_resource.h"
#include "vulkan/vulkan_fence.h"
#include "vulkan/vulkan_framebuffer.h"
#include "vulkan/vulkan_semaphore.h"

#include <core/platform_path.h>
#include <vulkan/vulkan_utils.h>

void GBufferPass::CreateResources(RenderGraph& graph)
{
	CreateCommandBuffers(graph);
	CreateSynchronizationPrimitives(graph);
	CreateTextures(graph);
	CreateRenderPass(graph);
	CreateShaders(graph);
	CreateMaterialLayout(graph);
	CreateMaterial(graph);
	CreateGraphicsPipeline(graph);
	CreateFramebuffers(graph);
}

void GBufferPass::Record(const FrameInfo& frameInfo, RenderGraph& graph)
{
	VulkanTexture2D& positionAttachmentResource = m_PositionTextureHandles[frameInfo.FrameIndex]->Get();
	VulkanTexture2D& normalAttachmentResource = m_NormalTextureHandles[frameInfo.FrameIndex]->Get();
	VulkanTexture2D& albedoAttachmentResource = m_AlbedoTextureHandles[frameInfo.FrameIndex]->Get();

	// Update internal host-side state to reflect the image transitions made during the render pass
	positionAttachmentResource.UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	normalAttachmentResource.UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	albedoAttachmentResource.UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VulkanCommandBuffer& cmd = m_CommandBufferHandles[frameInfo.FrameIndex]->Get();
	VulkanFramebuffer& fbo = m_FramebufferHandles[frameInfo.FrameIndex]->Get();

	auto& renderPass = m_RenderPassHandle->Get();
	auto& pipeline = m_PipelineHandle->Get();
	auto& fence = m_ResourcesInFlightFenceHandles[frameInfo.FrameIndex]->Get();

	cmd.WaitForCompletion(fence.GetHandle());

	cmd.Begin();
	{
		VkRenderPassBeginInfo gBufferRenderPassInfo{};
		gBufferRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		gBufferRenderPassInfo.renderPass = renderPass.GetHandle();
		gBufferRenderPassInfo.framebuffer = fbo.GetHandle();
		gBufferRenderPassInfo.renderArea.offset = {0, 0};
		auto attachmentExtent = VkExtent2D { fbo.Width(), fbo.Height() };
		gBufferRenderPassInfo.renderArea.extent = attachmentExtent;

		renderPass.BeginPass(cmd.GetHandle(), gBufferRenderPassInfo, attachmentExtent);
		{
			pipeline.Bind(cmd.GetHandle());
			for (auto& [id, gameObject] : frameInfo.ActiveScene.GameObjects)
			{
				auto& globalUbo = *graph.Get<VulkanBuffer>(GlobalUniformBufferResourceName, frameInfo.FrameIndex);
				gameObject.Render(cmd.GetHandle(), frameInfo.FrameIndex, globalUbo.DescriptorInfo());
			}
		}
		renderPass.EndPass(cmd.GetHandle());
	}
	cmd.End();

	// Update internal host-side state to reflect the image transitions made during the render pass
	positionAttachmentResource.UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	normalAttachmentResource.UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	albedoAttachmentResource.UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void GBufferPass::Submit(const FrameInfo& frameInfo, RenderGraph& graph)
{
	auto waitSemaphoreResource = graph.GetResourceHandle<SemaphoreResource>(
		SwapchainImageAvailableSemaphoreResourceName,
		m_uuid,
		frameInfo.FrameIndex);

	VulkanSemaphore& signalSemaphore = m_RenderCompleteSemaphoreHandles[frameInfo.FrameIndex]->Get();
	VulkanFence& fence = m_ResourcesInFlightFenceHandles[frameInfo.FrameIndex]->Get();
	VulkanCommandBuffer& cmd = m_CommandBufferHandles[frameInfo.FrameIndex]->Get();

	std::vector<VkSemaphore> waitSemaphores = { waitSemaphoreResource.Get()->Get().GetHandle() };
	std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkSemaphore> signalSemaphores = {signalSemaphore.GetHandle() };
	std::vector<VulkanCommandBuffer*> cmdBuffers = { &cmd };

	VulkanCommandBuffer::Submit(
		VulkanContext::Get().GraphicsQueue(),
		cmdBuffers,
		waitSemaphores,
		waitStages,
		signalSemaphores,
		fence.GetHandle());
}

void GBufferPass::CreateCommandBuffers(RenderGraph& graph)
{
	auto& context = VulkanContext::Get();

	auto createCommandBuffer =
		[](size_t index, const std::string& name, VkCommandPool pool, bool isPrimary)
	{
		auto commandBuffer = new VulkanCommandBuffer(pool, isPrimary, name);
		return std::make_unique<CommandBufferResource>(name, commandBuffer);
	};

	m_CommandBufferHandles = graph.CreateResources<CommandBufferResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		GBufferCommandBufferResourceName,
		createCommandBuffer,
		context.GraphicsCommandPool(),
		true);
}

void GBufferPass::CreateSynchronizationPrimitives(RenderGraph& graph)
{
	auto createSemaphore = [](size_t index, const std::string& name)
	{
		auto semaphore = new VulkanSemaphore(name);
		return std::make_unique<SemaphoreResource>(name, semaphore);
	};

	m_RenderCompleteSemaphoreHandles = graph.CreateResources<SemaphoreResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		GBufferRenderCompleteSemaphoreResourceName,
		createSemaphore);

	auto createFence = [](size_t index, const std::string& name)
	{
		auto fence = new VulkanFence(name, true);
		return std::make_unique<FenceResource>(name, fence);
	};

	m_ResourcesInFlightFenceHandles = graph.CreateResources<FenceResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		GBufferResourcesInFlightResourceName,
		createFence);
}

void GBufferPass::CreateTextures(RenderGraph& graph)
{
	auto& app = Application::Get();
	auto& swapchain = app.GetRenderer().GetSwapchainRenderer().GetSwapchain();

	TextureSpecification floatingPointAttachmentSpec
		{
			.Format = ImageFormat::RGBA16F,
			.Usage = TextureUsage::Attachment,
			.Width = swapchain.Width(),
			.Height = swapchain.Height(),
			.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			.SamplerSpec{.MinFilter = VK_FILTER_NEAREST, .MagFilter = VK_FILTER_NEAREST},
		};

	auto createPositionTexture =
		[&floatingPointAttachmentSpec](size_t index, const std::string& name)
	{
		floatingPointAttachmentSpec.DebugName = name;
		auto texture = VulkanTexture2D::CreateAttachment(floatingPointAttachmentSpec);
		auto* rawPointer = texture.release();

		return std::make_unique<TextureResource>(floatingPointAttachmentSpec.DebugName, rawPointer);
	};

	auto createNormalTexture =
		[&floatingPointAttachmentSpec](size_t index, const std::string& name)
	{
		floatingPointAttachmentSpec.DebugName = name;
		auto texture = VulkanTexture2D::CreateAttachment(floatingPointAttachmentSpec);
		auto* rawPointer = texture.release();
		return std::make_unique<TextureResource>(floatingPointAttachmentSpec.DebugName, rawPointer);
	};

	TextureSpecification byteAttachmentSpec
		{
			.Format = ImageFormat::RGBA,
			.Usage = TextureUsage::Attachment,
			.Width = swapchain.Width(),
			.Height = swapchain.Height(),
			.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			.SamplerSpec{.MinFilter = VK_FILTER_LINEAR, .MagFilter = VK_FILTER_LINEAR},
		};

	auto createAlbedoTexture =
		[&byteAttachmentSpec](size_t index, const std::string& name)
	{
		byteAttachmentSpec.DebugName = name;
		auto texture = VulkanTexture2D::CreateAttachment(byteAttachmentSpec);
		auto* rawPointer = texture.release();
		return std::make_unique<TextureResource>(byteAttachmentSpec.DebugName, rawPointer);
	};

	TextureSpecification depthTextureSpec
		{
			.Format = ImageFormat::DEPTH32F,
			.Usage = TextureUsage::Attachment,
			.Width = swapchain.Width(),
			.Height = swapchain.Height(),
			.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			.CreateSampler = false,
		};

	auto createDepthTexture =
		[&depthTextureSpec](size_t index, const std::string& name)
	{
		depthTextureSpec.DebugName = name;
		auto texture = VulkanTexture2D::CreateAttachment(depthTextureSpec);
		auto* rawPointer = texture.release();
		return std::make_unique<TextureResource>(depthTextureSpec.DebugName, rawPointer);
	};

	m_PositionTextureHandles = graph.CreateResources<TextureResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, GBufferPositionAttachmentTextureResourceName, createPositionTexture);
	m_NormalTextureHandles = graph.CreateResources<TextureResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, GBufferNormalAttachmentTextureResourceName, createNormalTexture);
	m_AlbedoTextureHandles = graph.CreateResources<TextureResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, GBufferAlbedoAttachmentTextureResourceName, createAlbedoTexture);
	m_DepthTextureHandles = graph.CreateResources<TextureResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, GBufferDepthAttachmentTextureResourceName, createDepthTexture);
}

void GBufferPass::CreateRenderPass(RenderGraph& graph)
{
	auto createRenderPass =
		[](size_t index, const std::string& name)
	{
		auto* renderPass = new VulkanRenderPass(name);

		// Position attachment
		renderPass->AddAttachment({.Type = AttachmentType::Color,
			.Format = ImageFormat::RGBA16F,
			.Samples = VK_SAMPLE_COUNT_1_BIT,
			.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
			.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.ClearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}});

		// Normal attachment
		renderPass->AddAttachment({.Type = AttachmentType::Color,
			.Format = ImageFormat::RGBA16F,
			.Samples = VK_SAMPLE_COUNT_1_BIT,
			.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
			.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.ClearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}});

		// Albedo attachment
		renderPass->AddAttachment({.Type = AttachmentType::Color,
			.Format = ImageFormat::RGBA,
			.Samples = VK_SAMPLE_COUNT_1_BIT,
			.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
			.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.ClearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}});

		// Depth attachment
		renderPass->AddAttachment({.Type = AttachmentType::Depth,
			.Format = ImageFormat::DEPTH32F,
			.Samples = VK_SAMPLE_COUNT_1_BIT,
			.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
			.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.FinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.ClearValue = {.depthStencil = {1.0f, 0}}});

		// Set up subpass
		SubpassDescription subpass;
		subpass.ColorAttachments = {0, 1, 2};	 // Position, Normal, Albedo
		subpass.DepthStencilAttachment = 3;		 // Depth
		renderPass->AddSubpass(subpass);

		renderPass->AddDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_DEPENDENCY_BY_REGION_BIT);

		renderPass->AddDependency(VK_SUBPASS_EXTERNAL, 0,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);

		renderPass->AddDependency(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_DEPENDENCY_BY_REGION_BIT);

		renderPass->Build();
		return std::make_unique<RenderPassObjectResource>(name, renderPass);
	};
	m_RenderPassHandle = graph.CreateResource<RenderPassObjectResource>(GBufferRenderPassResourceName, createRenderPass);
}

void GBufferPass::CreateShaders(RenderGraph& graph)
{
	auto createShader =
		[](size_t index, const std::string& name, const std::string& filePath, ShaderType shaderType)
	{
		auto* shader = new VulkanShader(filePath, shaderType);
		return std::make_unique<ShaderResource>(name, shader);
	};

	auto shaderDirectory = FileSystemUtil::GetShaderDirectory();
	auto gBufferVertPath = FileSystemUtil::PathToString(shaderDirectory / "gbuffer.vert");
	auto gBufferFragPath = FileSystemUtil::PathToString(shaderDirectory / "gbuffer.frag");

	m_VertexHandle = graph.CreateResource<ShaderResource>(GBufferVertexShaderResourceName, createShader, gBufferVertPath, ShaderType::Vertex);
	m_FragmentHandle = graph.CreateResource<ShaderResource>(GBufferFragmentShaderResourceName, createShader, gBufferFragPath, ShaderType::Fragment);
}

void GBufferPass::CreateMaterialLayout(RenderGraph& graph)
{
	auto createMaterialLayout =
		[](size_t index, const std::string& name, VulkanShader& vertShader, VulkanShader& fragShader)
	{
		auto materialLayout = new VulkanMaterialLayout(vertShader, fragShader, name);
		return std::make_unique<MaterialLayoutResource>(name, materialLayout);
	};

	auto& vertShader = m_VertexHandle.Get()->Get();
	auto& fragShader = m_FragmentHandle.Get()->Get();

	m_MaterialLayoutHandle = graph.CreateResource<MaterialLayoutResource>(
		GBufferMaterialLayoutResourceName,
		createMaterialLayout,
		vertShader,
		fragShader);
}

void GBufferPass::CreateMaterial(RenderGraph& graph)
{
	auto createMaterial =
		[](size_t index, const std::string& name, VulkanMaterialLayout& materialLayout)
	{
		auto material = new VulkanMaterial(materialLayout);
		return std::make_unique<MaterialResource>(name, material);
	};

	auto& materialLayout = m_MaterialLayoutHandle.Get()->Get();

	m_MaterialHandle = graph.CreateResource<MaterialResource>(
		GBufferMaterialResourceName,
		createMaterial,
		materialLayout);
}

void GBufferPass::CreateGraphicsPipeline(RenderGraph& graph)
{
	auto* renderPass = &m_RenderPassHandle.Get()->Get();
	auto* materialLayout = &m_MaterialLayoutHandle.Get()->Get();
	auto* vertShader = &m_VertexHandle.Get()->Get();
	auto* fragShader = &m_FragmentHandle.Get()->Get();

	auto createPipeline =
		[](size_t index, const std::string& name,
			VulkanRenderPass* renderPass,
			VulkanMaterialLayout* layout,
			VulkanShader* vertexShader,
			VulkanShader* fragmentShader)
	{
		auto builder = VulkanGraphicsPipelineBuilder(name)
						   .SetShaders(*vertexShader, *fragmentShader)
						   .SetVertexInputDescription({VulkanModel::Vertex::GetBindingDescriptions(), VulkanModel::Vertex::GetAttributeDescriptions()})
						   .SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
						   .SetPolygonMode(VK_POLYGON_MODE_FILL)
						   .SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
						   .SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
						   .SetDepthTesting(true, true, VK_COMPARE_OP_LESS_OR_EQUAL)
						   .SetRenderPass(renderPass)
						   .SetLayout(layout->GetPipelineLayout());

		auto pipeline = builder.Build();
		auto* rawPtr = pipeline.release();
		return std::make_unique<GraphicsPipelineResource>(name, rawPtr);
	};

	m_PipelineHandle = graph.CreateResource<GraphicsPipelineResource>(
		GBufferGraphicsPipelineResourceName,
		createPipeline,
		renderPass,
		materialLayout,
		vertShader,
		fragShader);
}

void GBufferPass::CreateFramebuffers(RenderGraph& graph)
{
	auto& swapchain = Application::Get().GetRenderer().VulkanSwapchain();
	VkExtent2D extent = swapchain.Extent();

	auto createFramebuffer =
		[this, &extent](uint32_t index, const std::string& name, VulkanRenderPass* renderPass)
	{
		auto positionView = m_PositionTextureHandles[index].Get()->Get().GetImage()->GetView();
		auto normalView = m_NormalTextureHandles[index].Get()->Get().GetImage()->GetView();
		auto albedoView = m_AlbedoTextureHandles[index].Get()->Get().GetImage()->GetView();
		auto depthView = m_DepthTextureHandles[index].Get()->Get().GetImage()->GetView();

		std::vector<VkImageView> attachments = {
			positionView->GetImageView(),
			normalView->GetImageView(),
			albedoView->GetImageView(),
			depthView->GetImageView()
		};

		auto framebuffer = new VulkanFramebuffer(name);
		framebuffer->Create(renderPass->GetHandle(), attachments, extent.width, extent.height);
		return std::make_unique<FramebufferResource>(name, framebuffer);
	};

	auto renderPassResource = &m_RenderPassHandle.Get()->Get();

	m_FramebufferHandles = graph.CreateResources<FramebufferResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		GBufferFramebufferResourceName,
		createFramebuffer,
		renderPassResource);
}

void GBufferPass::OnSwapchainResize(uint32_t width, uint32_t height, RenderGraph& graph)
{
	for (auto cmdBufferHandle : m_CommandBufferHandles)
	{
		auto& cmdResource = cmdBufferHandle.Get()->Get();

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(VulkanContext::Get().Device(), &fenceCreateInfo, nullptr, &fence));
		VK_CHECK_RESULT(cmdResource.InterruptAndReset(fence, true));
		vkDestroyFence(VulkanContext::Get().Device(), fence, nullptr);
	}

	auto freeFences = [](const std::shared_ptr<VulkanFence>& fence){};
	graph.InvalidateResource<FenceResource>(GBufferResourcesInFlightResourceName, freeFences);
	auto freeSemaphores = [](const std::shared_ptr<VulkanSemaphore>& semaphore){};
	graph.TryFreeResources<SemaphoreResource>(GBufferRenderCompleteSemaphoreResourceName, freeSemaphores);
	CreateSynchronizationPrimitives(graph);

	for(auto albedoHandle: m_AlbedoTextureHandles)
	{
		albedoHandle.Get()->Get().Resize(width, height);
	}

	for(auto positionHandle: m_PositionTextureHandles)
	{
		positionHandle.Get()->Get().Resize(width, height);
	}

	for(auto normalHandle: m_NormalTextureHandles)
	{
		normalHandle.Get()->Get().Resize(width, height);
	}

	for(auto depthHandle : m_DepthTextureHandles)
	{
		depthHandle.Get()->Get().Resize(width, height);
	}

	graph.TryFreeResources<RenderPassObjectResource>(GBufferRenderPassResourceName,  [](const std::shared_ptr<VulkanRenderPass>& renderPass){});
	CreateRenderPass(graph);

	graph.TryFreeResources<GraphicsPipelineObjectResource>(GBufferGraphicsPipelineResourceName,  [](const std::shared_ptr<VulkanGraphicsPipeline>& graphicsPipeline){});
	CreateGraphicsPipeline(graph);

	graph.TryFreeResources<FramebufferResource>(GBufferFramebufferResourceName,  [](const std::shared_ptr<VulkanFramebuffer>& framebuffer){});
	CreateFramebuffers(graph);
}
