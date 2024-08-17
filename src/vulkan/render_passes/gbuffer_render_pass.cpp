#include "gbuffer_render_pass.h"

#include "render_graph.h"

#include <vulkan/vulkan_utils.h>

#include <utility>

GBufferPass::GBufferPass()
{
}

void GBufferPass::CreateResources(RenderGraph& graph)
{
	auto& context = VulkanContext::Get();
	auto& app = Application::Get();
	auto& swapchain = app.GetRenderer().GetSwapchainRenderer().GetSwapchain();

	auto createCommandBuffer =
		[](size_t index, const std::string& resourceBaseName, VkCommandPool pool, bool isPrimary)
	{
		auto resourceName = resourceBaseName + " " + std::to_string(index);
		auto commandBuffer = std::make_shared<VulkanCommandBuffer>(pool, isPrimary, resourceName);
		return std::make_shared<CommandBufferResource>(resourceName, commandBuffer);
	};

	graph.CreateResources<CommandBufferResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		"G-Buffer Command Buffer",
		createCommandBuffer,
		context.GraphicsCommandPool(),
		true);

	auto swapchainSemaphores = graph.CreateResources<SemaphoreResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, "G-Buffer Render Complete Semaphore",
		[&context](size_t index, const std::string& baseName)
		{
			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkSemaphore semaphore;
			VK_CHECK_RESULT(vkCreateSemaphore(context.Device(), &semaphoreInfo, nullptr, &semaphore));
			return std::make_shared<SemaphoreResource>("SwapchainSemaphore_" + std::to_string(index), semaphore);
		});

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
		floatingPointAttachmentSpec.DebugName = "G-Buffer Position " + std::to_string(index);
		auto texture = std::make_shared<VulkanTexture2D>(floatingPointAttachmentSpec);
		return std::make_shared<TextureResource>(floatingPointAttachmentSpec.DebugName, texture);
	};

	auto createNormalTexture =
		[&floatingPointAttachmentSpec](size_t index, const std::string& resourceBaseName)
	{
		floatingPointAttachmentSpec.DebugName = "G-Buffer Normal " + std::to_string(index);
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
		byteAttachmentSpec.DebugName = "G-Buffer Albedo " + std::to_string(index);
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

	graph.CreateResources<TextureResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, "G-Buffer Position Attachment", createPositionTexture);
	graph.CreateResources<TextureResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, "G-Buffer Normal Attachment", createNormalTexture);
	graph.CreateResources<TextureResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, "G-Buffer Albedo Attachment", createAlbedoTexture);
	graph.CreateResources<TextureResource>(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT, "G-Buffer Depth Attachment", createDepthTexture);

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

	auto createShader =
		[](const std::string& resourceBaseName, const std::string& filePath, ShaderType shaderType)
	{
		auto shader = std::make_shared<VulkanShader>(filePath, shaderType);
		return std::make_shared<ShaderResource>(resourceBaseName, shader);
	};

	auto shaderDirectory = FileSystemUtil::GetShaderDirectory();
	auto gBufferVertPath = FileSystemUtil::PathToString(shaderDirectory / "gbuffer.vert");
	auto gBufferFragPath = FileSystemUtil::PathToString(shaderDirectory / "gbuffer.frag");

	auto vertHandle = graph.CreateResource<ShaderResource>("G-Buffer Vertex Shader", createShader, gBufferVertPath, ShaderType::Vertex);
	auto fragHandle = graph.CreateResource<ShaderResource>("G-Buffer Fragment Shader", createShader, gBufferFragPath, ShaderType::Fragment);
	auto vertResource = graph.GetResource<ShaderResource>(vertHandle);
	auto fragResource = graph.GetResource<ShaderResource>(fragHandle);

	auto materialLayoutHandle = graph.CreateResource<MaterialLayoutResource>(
		"G-Buffer Material Layout",
		[](const std::string& resourceBaseName, VulkanShader& vertShader, VulkanShader& fragShader)
		{
			auto materialLayout = std::make_shared<VulkanMaterialLayout>(vertShader, fragShader, resourceBaseName);
			return std::make_shared<MaterialLayoutResource>(resourceBaseName, materialLayout);
		},
		*vertResource->Get(),
		*fragResource->Get());
	auto materialLayout = graph.GetResource<MaterialLayoutResource>(materialLayoutHandle);


	graph.CreateResource<MaterialResource>(
		"G-Buffer Material Layout",
		[](const std::string& resourceBaseName, const std::shared_ptr<VulkanMaterialLayout>& layout)
		{
			auto material = std::make_shared<VulkanMaterial>(layout);
			return std::make_shared<MaterialResource>(resourceBaseName, material);
		},
		materialLayout->Get());

	auto renderPassHandle = graph.CreateResource<RenderPassObjectResource>("G-Buffer Render Pass", createRenderPass);
	auto renderPassResource = graph.GetResource<RenderPassObjectResource>(renderPassHandle);

	auto createPipeline =
		[](	const std::string& resourceBaseName,
			VulkanRenderPass* renderPass,
			VulkanMaterialLayout* layout,
			VulkanShader* vertexShader,
			VulkanShader* fragmentShader)
	{
		auto builder = VulkanGraphicsPipelineBuilder("G-Buffer Pipeline")
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

	graph.CreateResource<GraphicsPipelineObjectResource>(
		"G-Buffer Graphics Pipeline",
		createPipeline,
		renderPassResource->Get().get(),
		materialLayout->Get().get(),
		vertResource->Get().get(),
		fragResource->Get().get());
}

void GBufferPass::SetDependencies(RenderGraph& graph)
{
}

void GBufferPass::Invalidate()
{
	m_RenderPass.reset();
	m_Pipeline.reset();
	m_Framebuffers.clear();

	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();
}

void GBufferPass::Record(const FrameInfo& frameInfo, RenderGraph& graph)
{
	auto* positionAttachment = graph.GetResource<TextureResource>("G-Buffer Position Attachment", frameInfo.FrameIndex);
	auto* normalAttachment = graph.GetResource<TextureResource>("G-Buffer Normal Attachment", frameInfo.FrameIndex);
	auto* albedoAttachment = graph.GetResource<TextureResource>("G-Buffer Albedo Attachment", frameInfo.FrameIndex);
	auto* depthAttachment = graph.GetResource<TextureResource>("G-Buffer Albedo Attachment", frameInfo.FrameIndex);

	auto* cmdBuffer =

	// Update internal host-side state to reflect the image transitions made during the render pass
	positionAttachment->Get()->UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	normalAttachment->Get()->UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	albedoAttachment->Get()->UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto frameIndex = frameInfo.FrameIndex;
	auto cmd = m_CommandBuffers[frameIndex]->Get();
	auto fbo = m_Framebuffers[frameIndex]->Get();
	auto renderPass = m_RenderPass->Get();
	auto pipeline = m_Pipeline->Get();

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
				gameObject.Render(cmd->GetHandle(), frameIndex, globalUbo->DescriptorInfo());
			}
		}
		renderPass->EndPass(cmd->GetHandle());
	}
	cmd->End();

	// Update internal host-side state to reflect the image transitions made during the render pass
	for (size_t i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		m_PositionTextures[i]->Get()->UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_NormalTextures[i]->Get()->UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_AlbedoTextures[i]->Get()->UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

void GBufferPass::Submit(uint32_t frameIndex)
{
	// Submit G-Buffer pass
	std::vector<VkSemaphore> gBufferWaitSemaphores = { m_Renderer->ImageAvailableSemaphore(frameIndex) };
	std::vector<VkPipelineStageFlags> gBufferWaitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkSemaphore> gBufferSignalSemaphores = { m_PassCompleteSemaphores[frameIndex]->Get() };
	std::vector<VulkanCommandBuffer*> gBufferCommandBuffers = { m_CommandBuffers[frameIndex]->Get().get() };

	VulkanCommandBuffer::Submit(
		VulkanContext::Get().GraphicsQueue(),
		gBufferCommandBuffers,
		gBufferWaitSemaphores,
		gBufferWaitStages,
		gBufferSignalSemaphores);

}


void GBufferPass::CreateCommandBuffers()
{
	m_CommandBuffers.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);

	for(int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		auto gBufferCmdName = "G-Buffer Command Buffer " + std::to_string(i);
		auto cmdBuffer = std::make_shared<VulkanCommandBuffer>(VulkanContext::Get().GraphicsCommandPool(), true, gBufferCmdName);
		m_CommandBuffers[i] = std::make_shared<CommandBufferResource>(gBufferCmdName, cmdBuffer);
	}
}

void GBufferPass::CreateSynchronizationPrimitives()
{
	m_PassCompleteSemaphores.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for(int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		auto semaphoreName = "G-Buffer Render Complete Semaphore " + std::to_string(i);
		VkSemaphore semaphore;
		vkCreateSemaphore(VulkanContext::Get().Device(), &semaphoreInfo, nullptr, &semaphore);
		m_PassCompleteSemaphores[i] = std::make_shared<SemaphoreResource>(semaphoreName, semaphore);
	}
}


void GBufferPass::CreateGraphicsPipeline()
{
	auto builder = VulkanGraphicsPipelineBuilder("G-Buffer Pipeline")
					   .SetShaders(m_VertexShader, m_FragmentShader)
					   .SetVertexInputDescription({VulkanModel::Vertex::GetBindingDescriptions(), VulkanModel::Vertex::GetAttributeDescriptions()})
					   .SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
					   .SetPolygonMode(VK_POLYGON_MODE_FILL)
					   .SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
					   .SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
					   .SetDepthTesting(true, true, VK_COMPARE_OP_LESS_OR_EQUAL)
					   .SetRenderPass(m_RenderPass->Get().get())
					   .SetLayout(m_MaterialLayout->GetPipelineLayout());

	auto pipeline = builder.Build();
	m_Pipeline = std::make_shared<GraphicsPipelineObjectResource>("G-Buffer Pipeline", pipeline);
}

void GBufferPass::CreateFramebuffers()
{
	auto& swapchain = Application::Get().GetRenderer().VulkanSwapchain();
	VkExtent2D extent = swapchain.Extent();

	// Create G-Buffer framebuffers
	m_Framebuffers.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	for (uint32_t i = 0; i < m_Framebuffers.size(); i++)
	{
		auto positionTex = m_PositionTextures[i];
		auto normalTex = m_NormalTextures[i];
		auto albedoTex = m_AlbedoTextures[i];
		auto depthTex = m_DepthTextures[i];

		std::vector<VkImageView> attachments = {};

		auto debugName = "G-Buffer GetHandle " + std::to_string(i);
		auto framebuffer = std::make_shared<VulkanFramebuffer>(debugName);
		framebuffer->Create(m_RenderPass->Get()->GetHandle(), attachments, extent.width, extent.height);
		m_Framebuffers[i] = std::make_shared<FramebufferResource>(debugName, framebuffer);
	}
}

