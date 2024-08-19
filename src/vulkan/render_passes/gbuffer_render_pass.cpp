#include "gbuffer_render_pass.h"

#include "render_graph.h"

#include <vulkan/vulkan_utils.h>

#include <utility>


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
	ResourceHandle<TextureResource> positionHandle = m_PositionTextureHandles[frameInfo.FrameIndex];
	ResourceHandle<TextureResource> normalHandle = m_NormalTextureHandles[frameInfo.FrameIndex];
	ResourceHandle<TextureResource> albedoHandle = m_AlbedoTextureHandles[frameInfo.FrameIndex];
	auto* positionAttachmentResource = graph.GetResource<TextureResource>(positionHandle);
	auto* normalAttachmentResource = graph.GetResource<TextureResource>(normalHandle);
	auto* albedoAttachmentResource = graph.GetResource<TextureResource>(albedoHandle);

	// Update internal host-side state to reflect the image transitions made during the render pass
	positionAttachmentResource->Get()->UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	normalAttachmentResource->Get()->UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	albedoAttachmentResource->Get()->UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	ResourceHandle<CommandBufferResource> cmdHandle = m_CommandBufferHandles[frameInfo.FrameIndex];
	auto* cmdBufferResource = graph.GetResource<CommandBufferResource>(cmdHandle);

	ResourceHandle<FramebufferResource> fboHandle = m_FramebufferHandles[frameInfo.FrameIndex];
	auto* fboResource = graph.GetResource<FramebufferResource>(fboHandle);

	auto* renderPassResource = graph.GetResource<RenderPassObjectResource>(m_RenderPassHandle);
	auto* graphicsPipelineResource = graph.GetResource<GraphicsPipelineObjectResource>(m_PipelineHandle);

	auto cmd = cmdBufferResource->Get();
	auto fbo = fboResource->Get();
	auto renderPass = renderPassResource->Get();
	auto pipeline = graphicsPipelineResource->Get();

	cmd->Begin();
	{
		VkRenderPassBeginInfo gBufferRenderPassInfo{};
		gBufferRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		gBufferRenderPassInfo.renderPass = renderPass->GetHandle();
		gBufferRenderPassInfo.framebuffer = fbo->GetHandle();
		gBufferRenderPassInfo.renderArea.offset = {0, 0};
		auto attachmentExtent = VkExtent2D { fbo->Width(), fbo->Height() };
		gBufferRenderPassInfo.renderArea.extent = attachmentExtent;

		renderPass->BeginPass(cmd->GetHandle(), gBufferRenderPassInfo, attachmentExtent);
		{
			pipeline->Bind(cmd->GetHandle());
			for (auto& [id, gameObject] : frameInfo.ActiveScene.GameObjects)
			{
				auto globalUbo = frameInfo.GlobalUbo.lock();
				gameObject.Render(cmd->GetHandle(), frameInfo.FrameIndex, globalUbo->DescriptorInfo());
			}
		}
		renderPass->EndPass(cmd->GetHandle());
	}
	cmd->End();

	// Update internal host-side state to reflect the image transitions made during the render pass
	positionAttachmentResource->Get()->UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	normalAttachmentResource->Get()->UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	albedoAttachmentResource->Get()->UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void GBufferPass::Submit(const FrameInfo& frameInfo, RenderGraph& graph)
{
	auto waitSemaphoreResource = graph.GetResource<SemaphoreResource>("Swapchain Image Available Semaphore", frameInfo.FrameIndex);
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

void GBufferPass::CreateCommandBuffers(RenderGraph& graph)
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
		"G-Buffer Command Buffer",
		createCommandBuffer,
		context.GraphicsCommandPool(),
		true);
}

void GBufferPass::CreateSynchronizationPrimitives(RenderGraph& graph)
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
		"G-Buffer Render Complete Semaphore",
		createSemaphore);
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
		[&floatingPointAttachmentSpec](size_t index, const std::string& resourceBaseName)
	{
		floatingPointAttachmentSpec.DebugName = resourceBaseName + " " + std::to_string(index);
		auto texture = std::make_shared<VulkanTexture2D>(floatingPointAttachmentSpec);
		return std::make_shared<TextureResource>(floatingPointAttachmentSpec.DebugName, texture);
	};

	auto createNormalTexture =
		[&floatingPointAttachmentSpec](size_t index, const std::string& resourceBaseName)
	{
		floatingPointAttachmentSpec.DebugName = resourceBaseName + " " + std::to_string(index);
		auto texture = std::make_shared<VulkanTexture2D>(floatingPointAttachmentSpec);
		return std::make_shared<TextureResource>(floatingPointAttachmentSpec.DebugName, texture);
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
		[&byteAttachmentSpec](size_t index, const std::string& resourceBaseName)
	{
		byteAttachmentSpec.DebugName = resourceBaseName + " " + std::to_string(index);
		auto texture = std::make_shared<VulkanTexture2D>(byteAttachmentSpec);
		return std::make_shared<TextureResource>(byteAttachmentSpec.DebugName, texture);
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
		[&depthTextureSpec](size_t index, const std::string& resourceBaseName)
	{
		depthTextureSpec.DebugName = resourceBaseName + " " + std::to_string(index);
		auto texture = std::make_shared<VulkanTexture2D>(depthTextureSpec);
		return std::make_shared<TextureResource>(depthTextureSpec.DebugName, texture);
	};

	m_PositionTextureHandles = graph.CreateResources<TextureResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, "G-Buffer Position Attachment", createPositionTexture);
	m_NormalTextureHandles = graph.CreateResources<TextureResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, "G-Buffer Normal Attachment", createNormalTexture);
	m_AlbedoTextureHandles = graph.CreateResources<TextureResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, "G-Buffer Albedo Attachment", createAlbedoTexture);
	m_DepthTextureHandles = graph.CreateResources<TextureResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, "G-Buffer Depth Attachment", createDepthTexture);
}

void GBufferPass::CreateRenderPass(RenderGraph& graph)
{
	auto createRenderPass =
		[](const std::string& resourceBaseName)
	{
		auto renderPass = std::make_shared<VulkanRenderPass>(resourceBaseName);

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
		return std::make_shared<RenderPassObjectResource>(resourceBaseName, renderPass);
	};
	m_RenderPassHandle = graph.CreateResource<RenderPassObjectResource>("G-Buffer Render Pass", createRenderPass);
}

void GBufferPass::CreateShaders(RenderGraph& graph)
{
	auto createShader =
		[](const std::string& resourceBaseName, const std::string& filePath, ShaderType shaderType)
	{
		auto shader = std::make_shared<VulkanShader>(filePath, shaderType);
		return std::make_shared<ShaderResource>(resourceBaseName, shader);
	};

	auto shaderDirectory = FileSystemUtil::GetShaderDirectory();
	auto gBufferVertPath = FileSystemUtil::PathToString(shaderDirectory / "gbuffer.vert");
	auto gBufferFragPath = FileSystemUtil::PathToString(shaderDirectory / "gbuffer.frag");

	m_VertexHandle = graph.CreateResource<ShaderResource>("G-Buffer Vertex Shader", createShader, gBufferVertPath, ShaderType::Vertex);
	m_FragmentHandle = graph.CreateResource<ShaderResource>("G-Buffer Fragment Shader", createShader, gBufferFragPath, ShaderType::Fragment);
}

void GBufferPass::CreateMaterialLayout(RenderGraph& graph)
{
	auto createMaterialLayout =
		[](const std::string& resourceBaseName, VulkanShader& vertShader, VulkanShader& fragShader)
	{
		auto materialLayout = std::make_shared<VulkanMaterialLayout>(vertShader, fragShader, resourceBaseName);
		return std::make_shared<MaterialLayoutResource>(resourceBaseName, materialLayout);
	};

	auto vertResource = graph.GetResource<ShaderResource>(m_VertexHandle);
	auto fragResource = graph.GetResource<ShaderResource>(m_FragmentHandle);

	m_MaterialLayoutHandle = graph.CreateResource<MaterialLayoutResource>(
		"G-Buffer Material Layout",
		createMaterialLayout,
		*vertResource->Get(),
		*fragResource->Get());
}

void GBufferPass::CreateMaterial(RenderGraph& graph)
{
	auto createMaterial =
		[](const std::string& resourceBaseName, const std::shared_ptr<VulkanMaterialLayout>& materialLayout)
	{
		auto material = std::make_shared<VulkanMaterial>(materialLayout);
		return std::make_shared<MaterialResource>(resourceBaseName, material);
	};

	auto materialLayoutResource = graph.GetResource<MaterialLayoutResource>(m_MaterialLayoutHandle);

	m_MaterialHandle = graph.CreateResource<MaterialResource>(
		"G-Buffer Material",
		createMaterial,
		materialLayoutResource->Get());
}

void GBufferPass::CreateGraphicsPipeline(RenderGraph& graph)
{
	auto renderPassResource = graph.GetResource<RenderPassObjectResource>(m_RenderPassHandle);
	auto materialLayoutResource = graph.GetResource<MaterialLayoutResource>(m_MaterialLayoutHandle);
	auto vertShaderResource = graph.GetResource<ShaderResource>(m_VertexHandle);
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
		"G-Buffer Graphics Pipeline",
		createPipeline,
		renderPassResource->Get().get(),
		materialLayoutResource->Get().get(),
		vertShaderResource->Get().get(),
		fragShaderResource->Get().get());
}

void GBufferPass::CreateFramebuffers(RenderGraph& graph)
{
	auto& swapchain = Application::Get().GetRenderer().VulkanSwapchain();
	VkExtent2D extent = swapchain.Extent();

	auto createFramebuffer =
		[&extent, &graph](size_t index,
			const std::string& resourceBaseName,
			VulkanRenderPass* renderPass,
			const std::vector<ResourceHandle<TextureResource>>& positionTextureHandles,
			const std::vector<ResourceHandle<TextureResource>>& normalTextureHandles,
			const std::vector<ResourceHandle<TextureResource>>& albedoTextureHandles,
			const std::vector<ResourceHandle<TextureResource>>& depthTextureHandles)
	{

		auto positionView = graph.GetResource(positionTextureHandles[index])->Get()->GetImage()->GetView();
		auto normalView = graph.GetResource(normalTextureHandles[index])->Get()->GetImage()->GetView();
		auto albedoView = graph.GetResource(albedoTextureHandles[index])->Get()->GetImage()->GetView();
		auto depthView = graph.GetResource(depthTextureHandles[index])->Get()->GetImage()->GetView();

		std::vector<VkImageView> attachments = {
			positionView->GetImageView(),
			normalView->GetImageView(),
			albedoView->GetImageView(),
			depthView->GetImageView()
		};

		auto resourceName = resourceBaseName + " " + std::to_string(index);
		auto framebuffer = std::make_shared<VulkanFramebuffer>(resourceName);
		framebuffer->Create(renderPass->GetHandle(), attachments, extent.width, extent.height);
		return std::make_shared<FramebufferResource>(resourceBaseName, framebuffer);
	};

	auto renderPassResource = graph.GetResource(m_RenderPassHandle);

	m_FramebufferHandles = graph.CreateResources<FramebufferResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		"G-Buffer Framebuffer",
		createFramebuffer,
		renderPassResource->Get().get(),
		m_PositionTextureHandles,
		m_NormalTextureHandles,
		m_AlbedoTextureHandles,
		m_DepthTextureHandles);
}
