#include "scene_composition_pass.h"

#include "core/application.h"
#include "core/platform_path.h"
#include "vulkan/vulkan_semaphore.h"
#include "vulkan/vulkan_fence.h"
#include "render_graph.h"

void SceneCompositionPass::CreateResources(RenderGraph& graph)
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

void SceneCompositionPass::Record(const FrameInfo& frameInfo, RenderGraph& graph)
{
	ResourceHandle<TextureResource> colorAttachmentHandle = m_ColorAttachmentHandles[frameInfo.FrameIndex];
	auto* colorAttachmentResource = graph.GetResource<TextureResource>(colorAttachmentHandle);
	colorAttachmentResource->Get()->UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto* lightingColorAttachment = graph.GetResource<TextureResource>(LightingColorAttachmentResourceName, frameInfo.FrameIndex);

	ResourceHandle<CommandBufferResource> cmdHandle = m_CommandBufferHandles[frameInfo.FrameIndex];
	auto* cmdBufferResource = graph.GetResource<CommandBufferResource>(cmdHandle);

	ResourceHandle<FramebufferResource> fboHandle = m_FramebufferHandles[frameInfo.FrameIndex];
	auto* fboResource = graph.GetResource<FramebufferResource>(fboHandle);

	auto* renderPassResource = graph.GetResource<RenderPassObjectResource>(m_RenderPassHandle);
	auto* graphicsPipelineResource = graph.GetResource<GraphicsPipelineObjectResource>(m_PipelineHandle);
	auto* materialResource = graph.GetResource<MaterialResource>(m_MaterialHandle);
	auto* fenceResource = graph.GetResource<FenceResource>(m_ResourcesInFlightFenceHandles[frameInfo.FrameIndex]);

	auto cmd = cmdBufferResource->Get();
	cmd->WaitForCompletion(fenceResource->Get()->GetHandle());
	auto fbo = fboResource->Get();
	auto renderPass = renderPassResource->Get();
	auto pipeline = graphicsPipelineResource->Get();
	auto material = materialResource->Get();

	cmd->Begin();
	{
		colorAttachmentResource->Get()->UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderPassBeginInfo sceneCompositionRenderPassInfo{};
		sceneCompositionRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		sceneCompositionRenderPassInfo.renderPass = renderPass->GetHandle();
		sceneCompositionRenderPassInfo.framebuffer = fbo->GetHandle();
		sceneCompositionRenderPassInfo.renderArea.offset = {0, 0};
		auto attachmentExtent = VkExtent2D{fbo->Width(), fbo->Height()};
		sceneCompositionRenderPassInfo.renderArea.extent = attachmentExtent;

		pipeline->Bind(cmd->GetHandle());
		renderPass->BeginPass(cmd->GetHandle(), sceneCompositionRenderPassInfo, attachmentExtent);
		{
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
		colorAttachmentResource->Get()->UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	cmd->End();
}

void SceneCompositionPass::Submit(const FrameInfo& frameInfo, RenderGraph& graph)
{
	auto waitSemaphoreResource = graph.GetResource<SemaphoreResource>(LightingRenderCompleteSemaphoreResourceName, frameInfo.FrameIndex);
	auto signalSemaphoreResource = graph.GetResource(m_RenderCompleteSemaphoreHandles[frameInfo.FrameIndex]);
	auto cmdBufferResource = graph.GetResource(m_CommandBufferHandles[frameInfo.FrameIndex]);

	std::vector<VkSemaphore> waitSemaphores = {waitSemaphoreResource->Get()->GetHandle() };
	std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkSemaphore> signalSemaphores = {signalSemaphoreResource->Get()->GetHandle() };
	std::vector<VulkanCommandBuffer*> cmdBuffers = { &*cmdBufferResource->Get() };
	auto fenceResource = graph.GetResource(m_ResourcesInFlightFenceHandles[frameInfo.FrameIndex]);

	VulkanCommandBuffer::Submit(
		VulkanContext::Get().GraphicsQueue(),
		cmdBuffers,
		waitSemaphores,
		waitStages,
		signalSemaphores,
		fenceResource->Get()->GetHandle());
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
		SceneCompositionCommandBufferResourceName,
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
		SceneCompositionRenderCompleteSemaphoreResourceName,
		createSemaphore);

	auto createFence = [](size_t index, const std::string& baseName)
	{
		auto resourceName = baseName + std::to_string(index);
		// Start in signaled state
		auto fence = std::make_shared<VulkanFence>(resourceName, true);
		return std::make_shared<FenceResource>(resourceName, fence);
	};

	m_ResourcesInFlightFenceHandles = graph.CreateResources<FenceResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		SceneCompositionResourcesInFlightResourceName,
		createFence);
}

void SceneCompositionPass::CreateTextures(RenderGraph& graph)
{
	auto& app = Application::Get();
	auto& swapchain = app.GetRenderer().GetSwapchainRenderer().GetSwapchain();

	TextureSpecification byteAttachmentSpec
	{
		.Format = ImageFormat::RGBA,
		.Usage = TextureUsage::Attachment,
		.Width = swapchain.Width(),
		.Height = swapchain.Height(),
		.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.SamplerSpec{.MinFilter = VK_FILTER_LINEAR, .MagFilter = VK_FILTER_LINEAR},
	};

	auto createColorTexture =
		[&byteAttachmentSpec](size_t index, const std::string& resourceBaseName)
	{
		byteAttachmentSpec.DebugName = resourceBaseName + " " + std::to_string(index);
		auto texture = VulkanTexture2D::CreateAttachment(byteAttachmentSpec);
		return std::make_shared<TextureResource>(byteAttachmentSpec.DebugName, texture);
	};

	m_ColorAttachmentHandles = graph.CreateResources<TextureResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, SceneCompositionColorAttachmentResourceName, createColorTexture);
}

void SceneCompositionPass::CreateRenderPass(RenderGraph& graph)
{
	auto createRenderPass =
		[](const std::string& resourceBaseName)
	{
		auto renderPass = std::make_shared<VulkanRenderPass>(resourceBaseName);

		// Color attachment
		renderPass->AddAttachment({.Type = AttachmentType::Color,
			.Format = ImageFormat::RGBA,
			.Samples = VK_SAMPLE_COUNT_1_BIT,
			.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
			.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.ClearValue{.color = {0.0f, 0.0f, 0.0f, 1.0f}}});

		// Set up subpass
		SubpassDescription subpass;
		subpass.ColorAttachments = {0};	   // Color
		renderPass->AddSubpass(subpass);

		renderPass->AddDependency(
			VK_SUBPASS_EXTERNAL, 0,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_DEPENDENCY_BY_REGION_BIT);

		renderPass->AddDependency(0, VK_SUBPASS_EXTERNAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_DEPENDENCY_BY_REGION_BIT);

		renderPass->Build();
		return std::make_shared<RenderPassObjectResource>(resourceBaseName, renderPass);
	};

	m_RenderPassHandle = graph.CreateResource<RenderPassObjectResource>(SceneCompositionRenderPassResourceName, createRenderPass);
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
	m_FragmentHandle = graph.CreateResource<ShaderResource>(SceneCompositionFragmentShaderResourceName, createShader, compositionFragPath, ShaderType::Fragment);
}

void SceneCompositionPass::CreateMaterialLayout(RenderGraph& graph)
{
	auto createMaterialLayout =
		[](const std::string& resourceBaseName, VulkanShader& vertShader, VulkanShader& fragShader)
	{
		auto materialLayout = std::make_shared<VulkanMaterialLayout>(vertShader, fragShader, resourceBaseName);
		return std::make_shared<MaterialLayoutResource>(resourceBaseName, materialLayout);
	};

	auto vertResource = graph.GetResource<ShaderResource>(FullScreenQuadShaderResourceName);
	auto fragResource = graph.GetResource<ShaderResource>(m_FragmentHandle);

	m_MaterialLayoutHandle = graph.CreateResource<MaterialLayoutResource>(
		SceneCompositionMaterialLayoutResourceName,
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
		SceneCompositionMaterialResourceName,
		createMaterial,
		materialLayoutResource->Get());
}

void SceneCompositionPass::CreateGraphicsPipeline(RenderGraph& graph)
{
	auto sceneCompRenderPass = graph.GetResource<RenderPassObjectResource>(m_RenderPassHandle);
	auto materialLayoutResource = graph.GetResource<MaterialLayoutResource>(m_MaterialLayoutHandle);
	auto vertShaderResource = graph.GetResource<ShaderResource>(FullScreenQuadShaderResourceName);
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
						   .SetVertexInputDescription({})
						   .SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
						   .SetPolygonMode(VK_POLYGON_MODE_FILL)
						   .SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
						   .SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
						   .SetDepthTesting(false, false, VK_COMPARE_OP_LESS_OR_EQUAL)
						   .SetRenderPass(renderPass)
						   .SetLayout(layout->GetPipelineLayout());

		auto pipeline = builder.Build();
		return std::make_shared<GraphicsPipelineObjectResource>(resourceBaseName, pipeline);
	};

	m_PipelineHandle = graph.CreateResource<GraphicsPipelineObjectResource>(
		SceneCompositionGraphicsPipelineResourceName,
		createPipeline,
		sceneCompRenderPass->Get().get(),
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
			VulkanRenderPass* renderPass,
			const std::vector<ResourceHandle<TextureResource>>& colorTextureHandles)
	{

		auto colorView = graph.GetResource(colorTextureHandles[index])->Get()->GetImage()->GetView();

		std::vector<VkImageView> attachments = {
			colorView->GetImageView()
		};

		auto resourceName = resourceBaseName + " " + std::to_string(index);
		auto framebuffer = std::make_shared<VulkanFramebuffer>(resourceName);
		framebuffer->Create(renderPass->GetHandle(), attachments, extent.width, extent.height);
		return std::make_shared<FramebufferResource>(resourceBaseName, framebuffer);
	};

	auto renderPassResource = graph.GetResource(m_RenderPassHandle);

	m_FramebufferHandles = graph.CreateResources<FramebufferResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		SceneCompositionFramebufferResourceName,
		createFramebuffer,
		renderPassResource->Get().get(),
		m_ColorAttachmentHandles);
}
