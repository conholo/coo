#include "vulkan_simple_renderer.h"

#include "vulkan_renderer.h"

VulkanSimpleRenderer::VulkanSimpleRenderer(VulkanRenderer* renderer) : m_Renderer(renderer)
{
}

VulkanSimpleRenderer::~VulkanSimpleRenderer()
{
}

void VulkanSimpleRenderer::Initialize()
{
	CreateSimpleResources();
}

void VulkanSimpleRenderer::Shutdown()
{
	m_SimplePass.reset();
	m_SimplePipeline.reset();
	m_SimpleFramebuffers.clear();
	m_SimpleMaterialLayout.reset();
	m_SimpleBaseMaterial.reset();

	for (size_t i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (m_SimpleRenderFinishedSemaphores[i] != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(VulkanContext::Get().Device(), m_SimpleRenderFinishedSemaphores[i], nullptr);
			m_SimpleRenderFinishedSemaphores[i] = VK_NULL_HANDLE;
		}
	}
}

void VulkanSimpleRenderer::CreateSimpleTexture()
{
	constexpr bool Procedural = false;

	if(Procedural)
	{
		TextureSpecification textureSpec;
		textureSpec.Width = 256;
		textureSpec.Height = 256;
		textureSpec.Format = ImageFormat::RGBA;
		textureSpec.GenerateMips = true;
		textureSpec.DebugName = "Simple Texture";

		// Create a simple checkerboard pattern
		std::vector<uint32_t> pixels(256 * 256);
		for (int y = 0; y < 256; y++)
		{
			for (int x = 0; x < 256; x++)
			{
				pixels[y * 256 + x] = ((x & 32) ^ (y & 32)) ? 0xFFFFFFFF : 0xFF000000;
			}
		}
		Buffer textureData(pixels.data(), pixels.size() * sizeof(uint32_t));
		m_SimpleTextureA = VulkanTexture2D::CreateFromMemory(textureSpec, textureData);

	}
	else
	{
		TextureSpecification missingSpec
		{
			.Usage = TextureUsage::Texture,
			.DebugName = "Missing Texture",
		};
		m_SimpleTextureA = VulkanTexture2D::CreateFromFile(missingSpec, "../assets/textures/missing.png");
		TextureSpecification noiseSpec
		{
			.Usage = TextureUsage::Texture,
			.DebugName = "Blue Noise",
		};
		m_SimpleTextureB = VulkanTexture2D::CreateFromFile(noiseSpec, "../assets/textures/blue-noise.png");
	}
}

void VulkanSimpleRenderer::CreateSimpleResources()
{
	CreateSimpleTexture();
	CreateSimpleRenderPass();
	CreateSimplePipeline();
	CreateSimpleFramebuffers();

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	m_SimpleRenderFinishedSemaphores.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; i++)
		vkCreateSemaphore(VulkanContext::Get().Device(), &semaphoreInfo, nullptr, &m_SimpleRenderFinishedSemaphores[i]);
}

void VulkanSimpleRenderer::CreateSimpleRenderPass()
{
	m_SimplePass = std::make_unique<VulkanRenderPass>("Simple Render Pass");

	// Color attachment (swapchain image)
	m_SimplePass->AddAttachment({
		.Type = AttachmentType::Color,
		.Format = ImageUtils::VulkanFormatToImageFormat(m_Renderer->VulkanSwapchain().SwapchainImageFormat()),
		.Samples = VK_SAMPLE_COUNT_1_BIT,
		.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
		.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.ClearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}});

	// Set up subpass
	SubpassDescription subpass;
	subpass.ColorAttachments = {0};
	m_SimplePass->AddSubpass(subpass);

	// Add a single dependency
	m_SimplePass->AddDependency(
		VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_DEPENDENCY_BY_REGION_BIT);

	m_SimplePass->Build();
}

void VulkanSimpleRenderer::CreateSimplePipeline()
{
	if(!m_SimpleVertexShader || !m_SimpleFragmentShader)
	{
		m_SimpleVertexShader = std::make_shared<VulkanShader>("../assets/shaders/fsq.vert", ShaderType::Vertex);
		m_SimpleFragmentShader = std::make_shared<VulkanShader>("../assets/shaders/simple.frag", ShaderType::Fragment);
	}

	m_SimpleMaterialLayout = std::make_shared<VulkanMaterialLayout>(m_SimpleVertexShader, m_SimpleFragmentShader);
	m_SimpleBaseMaterial = std::make_shared<VulkanMaterial>(m_SimpleMaterialLayout);

	auto builder = VulkanGraphicsPipelineBuilder("Simple Pipeline")
					   .SetShaders(m_SimpleVertexShader, m_SimpleFragmentShader)
					   .SetVertexInputDescription()
					   .SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
					   .SetPolygonMode(VK_POLYGON_MODE_FILL)
					   .SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
					   .SetRenderPass(m_SimplePass.get())
					   .SetLayout(m_SimpleMaterialLayout->GetPipelineLayout());

	m_SimplePipeline = builder.Build();
}

void VulkanSimpleRenderer::CreateSimpleFramebuffers()
{
	VkExtent2D extent = m_Renderer->VulkanSwapchain().Extent();
	uint32_t imageCount = m_Renderer->VulkanSwapchain().ImageCount();

	m_SimpleFramebuffers.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; i++)
	{
		auto weakImage = m_Renderer->VulkanSwapchain().GetImage(i);
		auto sharedImage = weakImage.lock();

		std::vector<VkImageView> attachments;
		if (sharedImage)
			attachments.push_back(sharedImage->GetView()->GetImageView());

		m_SimpleFramebuffers[i] = std::make_unique<VulkanFramebuffer>("Simple Framebuffer " + std::to_string(i));
		m_SimpleFramebuffers[i]->Create(m_SimplePass->RenderPass(), attachments, extent.width, extent.height);
	}
}

void VulkanSimpleRenderer::Render(FrameInfo& frameInfo)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	vkBeginCommandBuffer(frameInfo.DrawCommandBuffer, &beginInfo);
	{
		uint32_t swapchainImageIndex = m_Renderer->GetCurrentSwapchainImageIndex();
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_SimplePass->RenderPass();
		renderPassInfo.framebuffer = m_SimpleFramebuffers[swapchainImageIndex]->Framebuffer();

		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = m_Renderer->VulkanSwapchain().Extent();

		m_SimplePass->BeginPass(frameInfo.DrawCommandBuffer, renderPassInfo, m_Renderer->VulkanSwapchain().Extent());
		{
			m_SimplePipeline->Bind(frameInfo.DrawCommandBuffer);
			m_SimpleBaseMaterial->UpdateDescriptorSets(frameInfo.FrameIndex,
				{
					{0,
						{
								{
									.binding = 0,
									.type = DescriptorUpdate::Type::Image,
									.imageInfo = m_SimpleTextureA->GetBaseViewDescriptorInfo()
								},
								{
									.binding = 1,
									.type = DescriptorUpdate::Type::Image,
									.imageInfo = m_SimpleTextureB->GetBaseViewDescriptorInfo()
								}
							}
						}
					});
			m_SimpleBaseMaterial->BindDescriptors(frameInfo.FrameIndex, frameInfo.DrawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
			vkCmdDraw(frameInfo.DrawCommandBuffer, 3, 1, 0, 0);
		}
		m_SimplePass->EndPass(frameInfo.DrawCommandBuffer);
	}
	vkEndCommandBuffer(frameInfo.DrawCommandBuffer);

	frameInfo.SignalForPresentation = m_SimpleRenderFinishedSemaphores[frameInfo.FrameIndex];
	frameInfo.WaitForGraphicsSubmit = m_Renderer->ImageAvailableSemaphore(frameInfo.FrameIndex);
}

void VulkanSimpleRenderer::Resize(uint32_t width, uint32_t height)
{
	Shutdown();
	Initialize();
}
