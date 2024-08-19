#include "scene_composition_pass.h"

#include "core/application.h"
#include "core/platform_path.h"
#include "render_graph.h"

void SceneCompositionPass::CreateResources(RenderGraph& graph)
{
	CreateCommandBuffers(graph);
	CreateSynchronizationPrimitives(graph);
	CreateRenderPass(graph);
	CreateShaders(graph);
	CreateMaterialLayout(graph);
	CreateMaterial(graph);
	CreateGraphicsPipeline(graph);
	CreateFramebuffers(graph);
}

void SceneCompositionPass::Record(const FrameInfo& frameInfo, RenderGraph& graph)
{
	auto* lightingColorAttachment = graph.GetResource<TextureResource>("Lighting Color Attachment", frameInfo.FrameIndex);
	ResourceHandle<CommandBufferResource> cmdHandle = m_CommandBufferHandles[frameInfo.FrameIndex];
	auto* cmdBufferResource = graph.GetResource<CommandBufferResource>(cmdHandle);

	ResourceHandle<FramebufferResource> fboHandle = m_FramebufferHandles[frameInfo.ImageIndex];
	auto* fboResource = graph.GetResource<FramebufferResource>(fboHandle);

	auto* renderPassResource = graph.GetResource<RenderPassObjectResource>(m_RenderPassHandle);
	auto* graphicsPipelineResource = graph.GetResource<GraphicsPipelineObjectResource>(m_PipelineHandle);
	auto* materialResource = graph.GetResource<MaterialResource>(m_MaterialHandle);

	auto cmd = cmdBufferResource->Get();
	auto fbo = fboResource->Get();
	auto renderPass = renderPassResource->Get();
	auto pipeline = graphicsPipelineResource->Get();
	auto material = materialResource->Get();

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
			pipeline->Bind(cmd->GetHandle());
			material->UpdateDescriptorSets(
				frameInfo.FrameIndex,
				{
					{0,
						{
							{
								.binding = 0,
								.type = DescriptorUpdate::Type::Image,
								.imageInfo = lightingColorAttachment->Get()->GetBaseViewDescriptorInfo()}
						}
					}
				});
			material->BindDescriptors(frameInfo.FrameIndex, cmd->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS);
			vkCmdDraw(cmd->GetHandle(), 3, 1, 0, 0);
		}
		renderPass->EndPass(cmd->GetHandle());
	}
	cmd->End();
}

void SceneCompositionPass::Submit(const FrameInfo& frameInfo, RenderGraph& graph)
{
	auto waitSemaphoreResource = graph.GetResource<SemaphoreResource>("Lighting Render Complete Semaphore", frameInfo.FrameIndex);
	auto signalSemaphoreResource = graph.GetResource(m_RenderCompleteSemaphoreHandles[frameInfo.FrameIndex]);
	auto cmdBufferResource = graph.GetResource(m_CommandBufferHandles[frameInfo.FrameIndex]);

	std::vector<VkSemaphore> waitSemaphores = {waitSemaphoreResource->Get()->GetHandle() };
	std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkSemaphore> signalSemaphores = {signalSemaphoreResource->Get()->GetHandle() };
	std::vector<VulkanCommandBuffer*> cmdBuffers = { &*cmdBufferResource->Get() };

	VulkanCommandBuffer::Submit(
		VulkanContext::Get().GraphicsQueue(),
		cmdBuffers,
		waitSemaphores,
		waitStages,
		signalSemaphores);
}

void SceneCompositionPass::CreateCommandBuffers(RenderGraph& graph)
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
		"Scene Composition Command Buffer",
		createCommandBuffer,
		context.GraphicsCommandPool(),
		true);
}

void SceneCompositionPass::CreateSynchronizationPrimitives(RenderGraph& graph)
{
	auto createSemaphore = [](size_t index, const std::string& baseName)
	{
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		auto resourceName = baseName + std::to_string(index);
		auto semaphore = std::make_shared<VulkanSemaphore>(resourceName);
		return std::make_shared<SemaphoreResource>(resourceName, semaphore);
	};

	m_RenderCompleteSemaphoreHandles = graph.CreateResources<SemaphoreResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		"Scene Composition Render Complete Semaphore",
		createSemaphore);
}

void SceneCompositionPass::CreateRenderPass(RenderGraph& graph)
{
	auto createRenderPass =
		[](const std::string& resourceBaseName)
	{
		auto renderPass = std::make_shared<VulkanRenderPass>(resourceBaseName);

		auto swapchainImageFormat = Application::Get().GetRenderer().VulkanSwapchain().SwapchainImageFormat();
		// Final color attachment (swapchain image)
		renderPass->AddAttachment(
			{
				.Type = AttachmentType::Color,
				.Format = ImageUtils::VulkanFormatToImageFormat(swapchainImageFormat),
				.Samples = VK_SAMPLE_COUNT_1_BIT,
				.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
				.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.ClearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
			});

		// Set up subpass
		SubpassDescription subpass;
		subpass.ColorAttachments = {0};	   // Color
		renderPass->AddSubpass(subpass);

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

	m_RenderPassHandle = graph.CreateResource<RenderPassObjectResource>("G-Buffer Render Pass", createRenderPass);
}

void SceneCompositionPass::CreateShaders(RenderGraph& graph)
{
	auto createShader =
		[](const std::string& resourceBaseName, const std::string& filePath, ShaderType shaderType)
	{
		auto shader = std::make_shared<VulkanShader>(filePath, shaderType);
		return std::make_shared<ShaderResource>(resourceBaseName, shader);
	};

	auto shaderDirectory = FileSystemUtil::GetShaderDirectory();
	auto compositionFragPath = FileSystemUtil::PathToString(shaderDirectory / "texture_display.frag");
	m_FragmentHandle = graph.CreateResource<ShaderResource>("Scene Composition Fragment Shader", createShader, compositionFragPath, ShaderType::Fragment);
}

void SceneCompositionPass::CreateMaterialLayout(RenderGraph& graph)
{
	auto createMaterialLayout =
		[](const std::string& resourceBaseName, VulkanShader& vertShader, VulkanShader& fragShader)
	{
		auto materialLayout = std::make_shared<VulkanMaterialLayout>(vertShader, fragShader, resourceBaseName);
		return std::make_shared<MaterialLayoutResource>(resourceBaseName, materialLayout);
	};

	auto vertResource = graph.GetResource<ShaderResource>("Full Screen Quad Vertex Shader");
	auto fragResource = graph.GetResource<ShaderResource>(m_FragmentHandle);

	m_MaterialLayoutHandle = graph.CreateResource<MaterialLayoutResource>(
		"Scene Composition Material Layout",
		createMaterialLayout,
		*vertResource->Get(),
		*fragResource->Get());
}

void SceneCompositionPass::CreateMaterial(RenderGraph& graph)
{
	auto createMaterial =
		[](const std::string& resourceBaseName, const std::shared_ptr<VulkanMaterialLayout>& materialLayout)
	{
		auto material = std::make_shared<VulkanMaterial>(materialLayout);
		return std::make_shared<MaterialResource>(resourceBaseName, material);
	};

	auto materialLayoutResource = graph.GetResource<MaterialLayoutResource>(m_MaterialLayoutHandle);

	m_MaterialHandle = graph.CreateResource<MaterialResource>(
		"Scene Composition Material",
		createMaterial,
		materialLayoutResource->Get());
}

void SceneCompositionPass::CreateGraphicsPipeline(RenderGraph& graph)
{
	auto renderPassResource = graph.GetResource<RenderPassObjectResource>(m_RenderPassHandle);
	auto materialLayoutResource = graph.GetResource<MaterialLayoutResource>(m_MaterialLayoutHandle);
	auto vertShaderResource = graph.GetResource<ShaderResource>("Full Screen Quad Vertex Shader");
	auto fragShaderResource = graph.GetResource<ShaderResource>(m_FragmentHandle);

	auto createPipeline =
		[](	const std::string& resourceBaseName,
			VulkanRenderPass* renderPass,
			VulkanMaterialLayout* layout,
			VulkanShader* vertexShader,
			VulkanShader* fragmentShader)
	{
		auto builder = VulkanGraphicsPipelineBuilder(resourceBaseName)
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
		return std::make_shared<GraphicsPipelineObjectResource>(resourceBaseName, pipeline);
	};

	m_PipelineHandle = graph.CreateResource<GraphicsPipelineObjectResource>(
		"Scene Composition Graphics Pipeline",
		createPipeline,
		renderPassResource->Get().get(),
		materialLayoutResource->Get().get(),
		vertShaderResource->Get().get(),
		fragShaderResource->Get().get());
}

void SceneCompositionPass::CreateFramebuffers(RenderGraph& graph)
{
	auto& swapchain = Application::Get().GetRenderer().VulkanSwapchain();
	VkExtent2D extent = swapchain.Extent();

	auto createFramebuffer =
		[&extent, &graph](
			size_t index,
			const std::string& resourceBaseName,
			VulkanRenderPass* renderPass)
	{
		auto viewResource = graph.GetResource<ImageViewResource>("Swapchain Image View", index);

		std::vector<VkImageView> attachments = {
			viewResource->Get()->GetImageView()
		};

		auto resourceName = resourceBaseName + " " + std::to_string(index);
		auto framebuffer = std::make_shared<VulkanFramebuffer>(resourceName);
		framebuffer->Create(renderPass->GetHandle(), attachments, extent.width, extent.height);
		return std::make_shared<FramebufferResource>(resourceBaseName, framebuffer);
	};

	auto renderPassResource = graph.GetResource(m_RenderPassHandle);

	m_FramebufferHandles = graph.CreateResources<FramebufferResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		"Scene Composition Framebuffer",
		createFramebuffer,
		renderPassResource->Get().get());
}
