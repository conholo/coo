#include "scene_composition_pass.h"

#include "core/application.h"
#include "core/platform_path.h"
#include "vulkan/vulkan_semaphore.h"
#include "vulkan/vulkan_fence.h"
#include "render_graph.h"
#include "render_graph_resource_declarations.h"

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
	VulkanTexture2D& colorAttachment = m_ColorAttachmentHandles[frameInfo.FrameIndex]->Get();
	colorAttachment.TransitionLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	TextureResource* lightingColorAttachmentHandle = graph.GetGlobalResourceHandle<TextureResource>(LightingColorAttachmentResourceName, frameInfo.FrameIndex).Get();
	VulkanCommandBuffer& cmd = m_CommandBufferHandles[frameInfo.FrameIndex]->Get();
	VulkanFramebuffer& fbo = m_FramebufferHandles[frameInfo.FrameIndex]->Get();

	VulkanRenderPass& renderPass = m_RenderPassHandle->Get();
	VulkanGraphicsPipeline& pipeline = m_PipelineHandle->Get();
	VulkanMaterial& material = m_MaterialHandle->Get();
	VulkanFence& fence = m_ResourcesInFlightFenceHandles[frameInfo.FrameIndex]->Get();

	cmd.WaitForCompletion(fence.GetHandle());

	cmd.Begin();
	{
		VkRenderPassBeginInfo sceneCompositionRenderPassInfo{};
		sceneCompositionRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		sceneCompositionRenderPassInfo.renderPass = renderPass.GetHandle();
		sceneCompositionRenderPassInfo.framebuffer = fbo.GetHandle();
		sceneCompositionRenderPassInfo.renderArea.offset = {0, 0};
		auto attachmentExtent = VkExtent2D{fbo.Width(), fbo.Height()};
		sceneCompositionRenderPassInfo.renderArea.extent = attachmentExtent;

		pipeline.Bind(cmd.GetHandle());
		renderPass.BeginPass(cmd.GetHandle(), sceneCompositionRenderPassInfo, attachmentExtent);
		{
			material.UpdateDescriptorSets(
				frameInfo.FrameIndex,
				{
					{0,
						{
							{
								.binding = 0,
								.type = DescriptorUpdate::Type::Image,
								.imageInfo = lightingColorAttachmentHandle->Get().GetBaseViewDescriptorInfo()}
						}
					}
				});
			material.BindDescriptors(frameInfo.FrameIndex, cmd.GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS);
			vkCmdDraw(cmd.GetHandle(), 3, 1, 0, 0);
		}
		renderPass.EndPass(cmd.GetHandle());
		colorAttachment.UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	cmd.End();
}

void SceneCompositionPass::Submit(const FrameInfo& frameInfo, RenderGraph& graph)
{
	VulkanSemaphore& waitSemaphore = graph.GetGlobalResourceHandle<SemaphoreResource>(LightingRenderCompleteSemaphoreResourceName, frameInfo.FrameIndex)->Get();
	VulkanSemaphore& signalSemaphore = m_RenderCompleteSemaphoreHandles[frameInfo.FrameIndex]->Get();
	VulkanCommandBuffer& cmd = m_CommandBufferHandles[frameInfo.FrameIndex]->Get();

	std::vector<VkSemaphore> waitSemaphores = { waitSemaphore.GetHandle() };
	std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkSemaphore> signalSemaphores = {signalSemaphore.GetHandle() };
	std::vector<VulkanCommandBuffer*> cmdBuffers = { &cmd };
	VulkanFence& fenceResource = m_ResourcesInFlightFenceHandles[frameInfo.FrameIndex]->Get();

	VulkanCommandBuffer::Submit(
		VulkanContext::Get().GraphicsQueue(),
		cmdBuffers,
		waitSemaphores,
		waitStages,
		signalSemaphores,
		fenceResource.GetHandle());
}

void SceneCompositionPass::CreateCommandBuffers(RenderGraph& graph)
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
		SceneCompositionCommandBufferResourceName,
		createCommandBuffer,
		context.GraphicsCommandPool(),
		true);
}

void SceneCompositionPass::CreateSynchronizationPrimitives(RenderGraph& graph)
{
	auto createSemaphore = [](size_t index, const std::string& name)
	{
		auto semaphore = new VulkanSemaphore(name);
		return std::make_unique<SemaphoreResource>(name, semaphore);
	};

	m_RenderCompleteSemaphoreHandles = graph.CreateResources<SemaphoreResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		SceneCompositionRenderCompleteSemaphoreResourceName,
		createSemaphore);

	auto createFence = [](size_t index, const std::string& name)
	{
		auto fence = new VulkanFence(name, true);
		return std::make_unique<FenceResource>(name, fence);
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
		[&byteAttachmentSpec](size_t index, const std::string& name)
	{
		byteAttachmentSpec.DebugName = name;
		auto texture = VulkanTexture2D::CreateAttachment(byteAttachmentSpec);
		auto* rawPtr = texture.release();
		return std::make_unique<TextureResource>(byteAttachmentSpec.DebugName, rawPtr);
	};

	m_ColorAttachmentHandles = graph.CreateResources<TextureResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		SceneCompositionColorAttachmentResourceName,
		createColorTexture);
}

void SceneCompositionPass::CreateRenderPass(RenderGraph& graph)
{
	auto createRenderPass =
		[](size_t index, const std::string& name)
	{
		auto renderPass = new VulkanRenderPass (name);

		// Color attachment
		renderPass->AddAttachment({
			.Type = AttachmentType::Color,
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
		return std::make_unique<RenderPassObjectResource>(name, renderPass);
	};

	m_RenderPassHandle = graph.CreateResource<RenderPassObjectResource>(SceneCompositionRenderPassResourceName, createRenderPass);
}

void SceneCompositionPass::CreateShaders(RenderGraph& graph)
{
	auto createShader =
		[](size_t index, const std::string& name, const std::string& filePath, ShaderType shaderType)
	{
		auto shader = new VulkanShader(filePath, shaderType);
		return std::make_unique<ShaderResource>(name, shader);
	};

	auto shaderDirectory = FileSystemUtil::GetShaderDirectory();
	auto compositionFragPath = FileSystemUtil::PathToString(shaderDirectory / "texture_display.frag");
	m_FragmentHandle = graph.CreateResource<ShaderResource>(SceneCompositionFragmentShaderResourceName, createShader, compositionFragPath, ShaderType::Fragment);
}

void SceneCompositionPass::CreateMaterialLayout(RenderGraph& graph)
{
	auto createMaterialLayout =
		[](size_t index, const std::string& name, VulkanShader& vertShader, VulkanShader& fragShader)
	{
		auto materialLayout = new VulkanMaterialLayout(vertShader, fragShader, name);
		return std::make_unique<MaterialLayoutResource>(name, materialLayout);
	};

	ResourceHandle<ShaderResource> vertResource = graph.GetGlobalResourceHandle<ShaderResource>(FullScreenQuadShaderResourceName);
	VulkanShader& vertShader = vertResource->Get();
	VulkanShader& fragShader = m_FragmentHandle->Get();

	m_MaterialLayoutHandle = graph.CreateResource<MaterialLayoutResource>(
		SceneCompositionMaterialLayoutResourceName,
		createMaterialLayout,
		vertShader,
		fragShader);
}

void SceneCompositionPass::CreateMaterial(RenderGraph& graph)
{
	auto createMaterial =
		[](size_t index, const std::string& name, VulkanMaterialLayout& materialLayoutRef)
	{
		auto material = new VulkanMaterial(materialLayoutRef);
		return std::make_unique<MaterialResource>(name, material);
	};

	VulkanMaterialLayout& materialLayoutRef = m_MaterialLayoutHandle->Get();

	m_MaterialHandle = graph.CreateResource<MaterialResource>(
		SceneCompositionMaterialResourceName,
		createMaterial,
		materialLayoutRef);
}

void SceneCompositionPass::CreateGraphicsPipeline(RenderGraph& graph)
{
	VulkanRenderPass& renderPass = m_RenderPassHandle->Get();
	VulkanMaterialLayout& materialLayout = m_MaterialLayoutHandle->Get();
	ResourceHandle<ShaderResource> vertShaderResource = graph.GetGlobalResourceHandle<ShaderResource>(FullScreenQuadShaderResourceName);
	VulkanShader& vertShader = vertShaderResource->Get();
	VulkanShader& fragShader = m_FragmentHandle->Get();

	auto createPipeline =
		[](	size_t index, const std::string& name,
			VulkanRenderPass* renderPass,
			VulkanMaterialLayout* layout,
			VulkanShader* vertexShader,
			VulkanShader* fragmentShader)
	{
		auto builder = VulkanGraphicsPipelineBuilder(name)
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
		auto* rawPtr = pipeline.release();
		return std::make_unique<GraphicsPipelineResource>(name, rawPtr);
	};

	m_PipelineHandle = graph.CreateResource<GraphicsPipelineResource>(
		SceneCompositionGraphicsPipelineResourceName,
		createPipeline,
		&renderPass,
		&materialLayout,
		&vertShader,
		&fragShader);
}

void SceneCompositionPass::CreateFramebuffers(RenderGraph& graph)
{
	auto& swapchain = Application::Get().GetRenderer().VulkanSwapchain();
	VkExtent2D extent = swapchain.Extent();

	auto createFramebuffer =
		[&extent, this](size_t index, const std::string& name, VulkanRenderPass* renderPass)
	{
		auto colorView = m_ColorAttachmentHandles[index]->Get().GetImage()->GetView();

		std::vector<VkImageView> attachments = {
			colorView->GetImageView()
		};

		auto framebuffer = new VulkanFramebuffer(name);
		framebuffer->Create(renderPass->GetHandle(), attachments, extent.width, extent.height);
		return std::make_unique<FramebufferResource>(name, framebuffer);
	};

	VulkanRenderPass& renderPass = m_RenderPassHandle->Get();
	m_FramebufferHandles = graph.CreateResources<FramebufferResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		SceneCompositionFramebufferResourceName,
		createFramebuffer,
		&renderPass);
}

void SceneCompositionPass::OnSwapchainResize(uint32_t width, uint32_t height, RenderGraph& graph)
{
	for (auto cmdBufferHandle : m_CommandBufferHandles)
	{
		VulkanCommandBuffer& cmd = cmdBufferHandle->Get();

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(VulkanContext::Get().Device(), &fenceCreateInfo, nullptr, &fence));
		VK_CHECK_RESULT(cmd.InterruptAndReset(fence, true));
		vkDestroyFence(VulkanContext::Get().Device(), fence, nullptr);
	}

	auto freeFences = [](const std::shared_ptr<VulkanFence>& fence){};
	graph.TryFreeResources<FenceResource>(SceneCompositionResourcesInFlightResourceName, freeFences);
	auto freeSemaphores = [](const std::shared_ptr<VulkanSemaphore>& semaphore){};
	graph.TryFreeResources<SemaphoreResource>(SceneCompositionRenderCompleteSemaphoreResourceName, freeSemaphores);
	CreateSynchronizationPrimitives(graph);

	for(auto colorHandle : m_ColorAttachmentHandles)
	{
		auto& texture = colorHandle->Get();
		texture.Resize(width, height);
	}

	graph.TryFreeResources<RenderPassObjectResource>(SceneCompositionRenderPassResourceName,  [](const std::shared_ptr<VulkanRenderPass>& renderPass){});
	CreateRenderPass(graph);

	graph.TryFreeResources<GraphicsPipelineObjectResource>(SceneCompositionGraphicsPipelineResourceName,  [](const std::shared_ptr<VulkanGraphicsPipeline>& graphicsPipeline){});
	CreateGraphicsPipeline(graph);

	graph.TryFreeResources<FramebufferResource>(SceneCompositionFramebufferResourceName,  [](const std::shared_ptr<VulkanFramebuffer>& framebuffer){});
	CreateFramebuffers(graph);
}