#include "vulkan_deferred_renderer.h"

#include "core/platform_path.h"
#include "vulkan_model.h"
#include "vulkan_renderer.h"
#include "vulkan_utils.h"

#include <vulkan/vulkan.h>

VulkanDeferredRenderer::VulkanDeferredRenderer(VulkanRenderer* renderer) : m_Renderer(renderer)
{
}

void VulkanDeferredRenderer::Initialize()
{
	CreateCommandBuffers();
	CreateSynchronizationPrimitives();
	CreateAttachmentTextures();
	CreateShaders();
	CreateMaterials();

	InvalidateGBufferPass();
	InvalidateLightingPass();
	InvalidateCompositionPass();
}

void VulkanDeferredRenderer::Shutdown()
{
	auto& contextRef = VulkanContext::Get();
	auto device = contextRef.Device();
	m_GBufferCommandBuffers.clear();
	m_LightingCommandBuffers.clear();

	for (size_t i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device, m_GBufferCompleteSemaphores[i], nullptr);
		m_GBufferCompleteSemaphores[i] = VK_NULL_HANDLE;
		vkDestroySemaphore(device, m_LightingCompleteSemaphores[i], nullptr);
		m_LightingCompleteSemaphores[i] = VK_NULL_HANDLE;
	}
	m_GBufferCompleteSemaphores.clear();
	m_LightingCompleteSemaphores.clear();

	// Textures/Attachments
	m_GBufferTextures.clear();
	m_LightingTextures.clear();

	// Render Passes
	m_GBufferPass.reset();
	m_GBufferPass = nullptr;
	m_LightingPass.reset();
	m_LightingPass = nullptr;
	m_CompositionPass.reset();
	m_CompositionPass = nullptr;

	// Pipelines
	m_GBufferPipeline.reset();
	m_GBufferPipeline = nullptr;

	m_LightingPipeline.reset();
	m_LightingPipeline = nullptr;

	m_CompositionPipeline.reset();
	m_CompositionPipeline = nullptr;

	// Framebuffers
	m_GBufferFramebuffers.clear();
	m_LightingFramebuffers.clear();
	m_CompositionFramebuffers.clear();

	// Materials/Layouts
	m_GBufferMaterialLayout.reset();
	m_GBufferMaterialLayout = nullptr;
	m_GBufferBaseMaterial.reset();
	m_GBufferBaseMaterial = nullptr;

	m_LightingMaterialLayout.reset();
	m_LightingMaterialLayout = nullptr;
	m_LightingMaterial.reset();
	m_LightingMaterial = nullptr;

	m_CompositionMaterialLayout.reset();
	m_CompositionMaterialLayout = nullptr;
	m_CompositionMaterial.reset();
	m_CompositionMaterial = nullptr;

	// Shaders
	m_FullScreenQuadVertexShader.reset();
	m_FullScreenQuadVertexShader = nullptr;

	m_GBufferVertexShader.reset();
	m_GBufferVertexShader = nullptr;

	m_GBufferFragmentShader.reset();
	m_GBufferFragmentShader = nullptr;

	m_LightingFragmentShader.reset();
	m_LightingFragmentShader = nullptr;

	m_CompositionFragmentShader.reset();
	m_CompositionFragmentShader = nullptr;
}

void VulkanDeferredRenderer::RegisterGameObject(GameObject& gameObjectRef)
{
	gameObjectRef.Material = m_GBufferBaseMaterial->Clone();
}

void VulkanDeferredRenderer::CreateCommandBuffers()
{
	m_GBufferCommandBuffers.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	m_LightingCommandBuffers.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);

	for(int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		auto gBufferCmdName = "GBuffer Command Buffer " + std::to_string(i);
		m_GBufferCommandBuffers[i] = std::make_unique<VulkanCommandBuffer>(VulkanContext::Get().GraphicsCommandPool(), true, gBufferCmdName);

		auto lightingCmdName = "Lighting Command Buffer " + std::to_string(i);
		m_LightingCommandBuffers[i] = std::make_unique<VulkanCommandBuffer>(VulkanContext::Get().GraphicsCommandPool(), true, lightingCmdName);
	}
}

void VulkanDeferredRenderer::CreateSynchronizationPrimitives()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	m_GBufferCompleteSemaphores.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	m_LightingCompleteSemaphores.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkCreateSemaphore(VulkanContext::Get().Device(), &semaphoreInfo, nullptr, &m_GBufferCompleteSemaphores[i]);
		vkCreateSemaphore(VulkanContext::Get().Device(), &semaphoreInfo, nullptr, &m_LightingCompleteSemaphores[i]);
	}
}

void VulkanDeferredRenderer::CreateAttachmentTextures()
{
	CreateGBufferTextures();
	CreateLightingTextures();
}

void VulkanDeferredRenderer::CreateShaders()
{
	auto shaderDirectory = FileSystemUtil::GetShaderDirectory();
	auto fsqPath = FileSystemUtil::PathToString(shaderDirectory / "fsq.vert");
	auto gBufferVertPath = FileSystemUtil::PathToString(shaderDirectory / "gbuffer.vert");
	auto gBufferFragPath = FileSystemUtil::PathToString(shaderDirectory / "gbuffer.frag");
	auto lightingFragPath = FileSystemUtil::PathToString(shaderDirectory / "lighting.frag");
	auto compositionFragPath = FileSystemUtil::PathToString(shaderDirectory / "texture_display.frag");

	m_FullScreenQuadVertexShader = std::make_shared<VulkanShader>(fsqPath, ShaderType::Vertex);
	m_GBufferVertexShader = std::make_shared<VulkanShader>(gBufferVertPath, ShaderType::Vertex);
	m_GBufferFragmentShader = std::make_shared<VulkanShader>(gBufferFragPath, ShaderType::Fragment);
	m_LightingFragmentShader = std::make_shared<VulkanShader>(lightingFragPath, ShaderType::Fragment);
	m_CompositionFragmentShader = std::make_shared<VulkanShader>(compositionFragPath, ShaderType::Fragment);
}

void VulkanDeferredRenderer::CreateMaterials()
{
	m_GBufferMaterialLayout = std::make_shared<VulkanMaterialLayout>(m_GBufferVertexShader, m_GBufferFragmentShader, "GBuffer Material Layout");
	m_GBufferBaseMaterial = std::make_shared<VulkanMaterial>(m_GBufferMaterialLayout);

	m_LightingMaterialLayout = std::make_shared<VulkanMaterialLayout>(m_FullScreenQuadVertexShader, m_LightingFragmentShader, "Lighting Material Layout");
	m_LightingMaterial = std::make_shared<VulkanMaterial>(m_LightingMaterialLayout);

	m_CompositionMaterialLayout = std::make_shared<VulkanMaterialLayout>(m_FullScreenQuadVertexShader, m_CompositionFragmentShader, "Composition Material Layout");
	m_CompositionMaterial = std::make_shared<VulkanMaterial>(m_CompositionMaterialLayout);
}

void VulkanDeferredRenderer::InvalidateGBufferPass()
{
	m_GBufferPass.reset();
	m_GBufferPipeline.reset();
	m_GBufferFramebuffers.clear();

	CreateGBufferRenderPass();
	CreateGBufferPipeline();
	CreateGBufferFramebuffers();
}

void VulkanDeferredRenderer::CreateGBufferTextures()
{
	TextureSpecification positionTextureSpec{
		.Format = ImageFormat::RGBA16F,
		.Usage = TextureUsage::Attachment,
		.Width = m_Renderer->VulkanSwapchain().Width(),
		.Height = m_Renderer->VulkanSwapchain().Height(),
		.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.SamplerSpec{.MinFilter = VK_FILTER_NEAREST, .MagFilter = VK_FILTER_NEAREST},
		.DebugName = "G-Buffer Position",
	};
	for (int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		positionTextureSpec.DebugName = "G-Buffer Position " + std::to_string(i);
		m_GBufferTextures[i].push_back(VulkanTexture2D::CreateAttachment(positionTextureSpec));
	}

	TextureSpecification normalTextureSpec{
		.Format = ImageFormat::RGBA16F,
		.Usage = TextureUsage::Attachment,
		.Width = m_Renderer->VulkanSwapchain().Width(),
		.Height = m_Renderer->VulkanSwapchain().Height(),
		.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.SamplerSpec{.MinFilter = VK_FILTER_NEAREST, .MagFilter = VK_FILTER_NEAREST},
		.DebugName = "G-Buffer Normal",
	};
	for (int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		normalTextureSpec.DebugName = "G-Buffer Normal " + std::to_string(i);
		m_GBufferTextures[i].push_back(VulkanTexture2D::CreateAttachment(normalTextureSpec));
	}

	TextureSpecification colorTextureSpec{
		.Format = ImageFormat::RGBA,
		.Usage = TextureUsage::Attachment,
		.Width = m_Renderer->VulkanSwapchain().Width(),
		.Height = m_Renderer->VulkanSwapchain().Height(),
		.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.SamplerSpec{.MinFilter = VK_FILTER_LINEAR, .MagFilter = VK_FILTER_LINEAR},
		.DebugName = "G-Buffer Color",
	};
	for (int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		colorTextureSpec.DebugName = "G-Buffer Color " + std::to_string(i);
		m_GBufferTextures[i].push_back(VulkanTexture2D::CreateAttachment(colorTextureSpec));
	}

	TextureSpecification depthTextureSpec{
		.Format = ImageFormat::DEPTH32F,
		.Usage = TextureUsage::Attachment,
		.Width = m_Renderer->VulkanSwapchain().Width(),
		.Height = m_Renderer->VulkanSwapchain().Height(),
		.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.CreateSampler = false,
		.DebugName = "G-Buffer Depth",
	};

	for (int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		depthTextureSpec.DebugName = "G-Buffer Depth " + std::to_string(i);
		m_GBufferTextures[i].push_back(VulkanTexture2D::CreateAttachment(depthTextureSpec));
	}
}

void VulkanDeferredRenderer::CreateGBufferRenderPass()
{
	m_GBufferPass = std::make_unique<VulkanRenderPass>("G-Buffer Render Pass");

	// Position attachment
	m_GBufferPass->AddAttachment({.Type = AttachmentType::Color,
		.Format = ImageFormat::RGBA16F,
		.Samples = VK_SAMPLE_COUNT_1_BIT,
		.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
		.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.ClearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}});

	// Normal attachment
	m_GBufferPass->AddAttachment({.Type = AttachmentType::Color,
		.Format = ImageFormat::RGBA16F,
		.Samples = VK_SAMPLE_COUNT_1_BIT,
		.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
		.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.ClearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}});

	// Albedo attachment
	m_GBufferPass->AddAttachment({.Type = AttachmentType::Color,
		.Format = ImageFormat::RGBA,
		.Samples = VK_SAMPLE_COUNT_1_BIT,
		.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
		.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.ClearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}});

	// Depth attachment
	m_GBufferPass->AddAttachment({.Type = AttachmentType::Depth,
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
	m_GBufferPass->AddSubpass(subpass);

	/*
	 * External -> G-Buffer Color Attachments Writes
	 *
	 * The latest stage that worked on the color attachments of the G-Buffer that must be waited on is either the FRAGMENT_SHADER
	 * (read by lighting pass) or BOTTOM_OF_THE_PIPE (? is this really necessary though ?) as a catch-all for the first frame
	 * where nothing worked on the attachments to begin with.  Once the src work is completed, the earliest stage that this pass
	 * will work on the attachment is in the COLOR_ATTACHMENT_OUTPUT stage where it will write to the attachment.
	 */
	m_GBufferPass->AddDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_DEPENDENCY_BY_REGION_BIT);

	/*
	 * External -> G-Buffer Depth/Stencil Attachment Dependency
	 *
	 * The latest stage that worked on the depth/stencil attachment of the G-Buffer that must be waited on is the early/late
	 * fragment stage tests where the attachments were written to.
	 * Once the src work is completed, the earliest stage that this pass will work on the depth/stencil is in the same part of the
	 * pipeline (early/late fragment tests) where the attachment could either be read from or written to.
	 */
	m_GBufferPass->AddDependency(VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);

	/*
	 * G-Buffer Color Attachment Writes -> G-Buffer Color Attachment Reads
	 *
	 * The latest stage that worked on the depth/stencil attachment of the G-Buffer that must be waited on is the early/late
	 * fragment stage tests where the attachments were written to.
	 * Once the src work is completed, the earliest stage that this pass will work on the depth/stencil is in the same part of the
	 * pipeline (early/late fragment tests) where the attachment could either be read from or written to.
	 */
	m_GBufferPass->AddDependency(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_DEPENDENCY_BY_REGION_BIT);

	m_GBufferPass->Build();
}

void VulkanDeferredRenderer::CreateGBufferPipeline()
{
	auto builder = VulkanGraphicsPipelineBuilder("G-Buffer Pipeline")
					   .SetShaders(m_GBufferVertexShader, m_GBufferFragmentShader)
					   .SetVertexInputDescription(
						   {VulkanModel::Vertex::GetBindingDescriptions(), VulkanModel::Vertex::GetAttributeDescriptions()})
					   .SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
					   .SetPolygonMode(VK_POLYGON_MODE_FILL)
					   .SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
					   .SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
					   .SetDepthTesting(true, true, VK_COMPARE_OP_LESS_OR_EQUAL)
					   .SetRenderPass(m_GBufferPass.get())
					   .SetLayout(m_GBufferMaterialLayout->GetPipelineLayout());

	m_GBufferPipeline = builder.Build();
}

void VulkanDeferredRenderer::CreateGBufferFramebuffers()
{
	VkExtent2D extent = m_Renderer->VulkanSwapchain().Extent();

	// Create G-Buffer framebuffers
	m_GBufferFramebuffers.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	for (uint32_t i = 0; i < m_GBufferFramebuffers.size(); i++)
	{
		std::vector<VkImageView> attachments;
		for (const auto& texture : m_GBufferTextures[i])
			attachments.push_back(texture->GetImage()->GetView()->GetImageView());

		m_GBufferFramebuffers[i] = std::make_unique<VulkanFramebuffer>("G-Buffer GetHandle " + std::to_string(i));
		m_GBufferFramebuffers[i]->Create(m_GBufferPass->GetHandle(), attachments, extent.width, extent.height);
	}
}

/*
 * Lighting
 */
void VulkanDeferredRenderer::InvalidateLightingPass()
{
	m_LightingPass.reset();
	m_LightingPipeline.reset();
	m_LightingFramebuffers.clear();

	CreateLightingRenderPass();
	CreateLightingPipeline();
	CreateLightingFramebuffers();
}

void VulkanDeferredRenderer::CreateLightingTextures()
{
	TextureSpecification textureSpec{
		.Format = ImageFormat::RGBA,
		.Usage = TextureUsage::Attachment,
		.Width = m_Renderer->VulkanSwapchain().Width(),
		.Height = m_Renderer->VulkanSwapchain().Height(),
		.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.SamplerSpec{.MinFilter = VK_FILTER_LINEAR, .MagFilter = VK_FILTER_LINEAR},
		.DebugName = "Lighting Color",
	};
	for (int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		textureSpec.DebugName = "Lighting Color Attachment " + std::to_string(i);
		m_LightingTextures[i] = VulkanTexture2D::CreateAttachment(textureSpec);
	}
}

void VulkanDeferredRenderer::CreateLightingRenderPass()
{
	m_LightingPass = std::make_unique<VulkanRenderPass>("Lighting Render Pass");

	// Color attachment
	m_LightingPass->AddAttachment({.Type = AttachmentType::Color,
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
	m_LightingPass->AddSubpass(subpass);

	/*
	 * External -> Lighting Color Attachment Writes
	 *
	 * The latest stage that worked on the lighting color attachment that must be waited on is either the FRAGMENT_SHADER stage
	 * (read by composition pass) or BOTTOM_OF_THE_PIPE (? is this really necessary though ?) as a catch-all for the first frame
	 * where nothing worked on the attachment to begin with.  Once the src work is completed, the earliest stage that this pass will
	 * work on the attachment is in the COLOR_ATTACHMENT_OUTPUT stage where it will write to the attachment.
	 */
	m_LightingPass->AddDependency(
		VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_DEPENDENCY_BY_REGION_BIT);

	/*
	 * Lighting Color Attachment Writes -> Lighting Color Attachment Reads
	 *
	 * Wait on the color attachment writes to the lighting image from the previous dependency.
	 * Once the writes are complete, this pass will prepare the attachment for fragment shader reads.
	 */
	m_LightingPass->AddDependency(0, VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_DEPENDENCY_BY_REGION_BIT);

	m_LightingPass->Build();
}

void VulkanDeferredRenderer::CreateLightingPipeline()
{
	auto builder = VulkanGraphicsPipelineBuilder("Lighting Pipeline")
					   .SetShaders(m_FullScreenQuadVertexShader, m_LightingFragmentShader)
					   .SetVertexInputDescription()
					   .SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
					   .SetPolygonMode(VK_POLYGON_MODE_FILL)
					   .SetCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
					   .SetRenderPass(m_LightingPass.get())
					   .SetDepthTesting(false, false, VK_COMPARE_OP_ALWAYS)
					   .SetLayout(m_LightingMaterialLayout->GetPipelineLayout());

	m_LightingPipeline = builder.Build();
}

void VulkanDeferredRenderer::CreateLightingFramebuffers()
{
	VkExtent2D extent = m_Renderer->VulkanSwapchain().Extent();

	m_LightingFramebuffers.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	for (uint32_t i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		std::vector<VkImageView> attachments{m_LightingTextures[i]->GetImage()->GetView()->GetImageView()};
		m_LightingFramebuffers[i] = std::make_unique<VulkanFramebuffer>("Lighting GetHandle " + std::to_string(i));
		m_LightingFramebuffers[i]->Create(m_LightingPass->GetHandle(), attachments, extent.width, extent.height);
	}
}

void VulkanDeferredRenderer::InvalidateCompositionPass()
{
	m_CompositionPass.reset();
	m_CompositionPipeline.reset();
	m_CompositionFramebuffers.clear();

	CreateCompositionRenderPass();
	CreateCompositionPipeline();
	CreateCompositionFramebuffers();
}

void VulkanDeferredRenderer::CreateCompositionRenderPass()
{
	m_CompositionPass = std::make_unique<VulkanRenderPass>("Composition Render Pass");

	auto swapchainImageFormat = Application::Get().GetRenderer().VulkanSwapchain().SwapchainImageFormat();
	// Final color attachment (swapchain image)
	m_CompositionPass->AddAttachment(
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

	SubpassDescription subpass;
	subpass.ColorAttachments = {0};
	m_CompositionPass->AddSubpass(subpass);

	m_CompositionPass->AddDependency(
		VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_DEPENDENCY_BY_REGION_BIT);

	m_CompositionPass->AddDependency(
		0, VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
		VK_DEPENDENCY_BY_REGION_BIT);

	m_CompositionPass->Build();
}

void VulkanDeferredRenderer::CreateCompositionPipeline()
{
	auto builder = VulkanGraphicsPipelineBuilder("Composition Pipeline")
					   .SetShaders(m_FullScreenQuadVertexShader, m_CompositionFragmentShader)
					   .SetVertexInputDescription()
					   .SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
					   .SetPolygonMode(VK_POLYGON_MODE_FILL)
					   .SetCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
					   .SetRenderPass(m_CompositionPass.get())
					   .SetDepthTesting(false, false, VK_COMPARE_OP_ALWAYS)
					   .SetLayout(m_CompositionMaterialLayout->GetPipelineLayout());

	m_CompositionPipeline = builder.Build();
}

void VulkanDeferredRenderer::CreateCompositionFramebuffers()
{
	VkExtent2D extent = Application::Get().GetRenderer().VulkanSwapchain().Extent();
	uint32_t imageCount = Application::Get().GetRenderer().VulkanSwapchain().ImageCount();

	m_CompositionFramebuffers.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; i++)
	{
		auto weakImage = Application::Get().GetRenderer().VulkanSwapchain().GetImage(i);
		auto sharedImage = weakImage.lock();

		std::vector<VkImageView> attachments;
		if (sharedImage)
			attachments.push_back(sharedImage->GetView()->GetImageView());

		m_CompositionFramebuffers[i] = std::make_unique<VulkanFramebuffer>("Composition GetHandle " + std::to_string(i));
		m_CompositionFramebuffers[i]->Create(m_CompositionPass->GetHandle(), attachments, extent.width, extent.height);
	}
}

/*
 *  A simple look at how rendering a few frames might look.
 *
 *  Frame 1:

	Acquire swapchain image (let's say index 0)
	Render G-Buffer using m_GBufferFramebuffers[0]
	Render Lighting using m_LightingFramebuffers[0]
	Render Composition to m_CompositionFramebuffers[0] (which uses swapchain image 0)
	Present swapchain image 0

	Frame 2:

	Acquire swapchain image (let's say index 1)
	Render G-Buffer using m_GBufferFramebuffers[1]
	Render Lighting using m_LightingFramebuffers[1]
	Render Composition to m_CompositionFramebuffers[1] (which uses swapchain image 1)
	Present swapchain image 1

	Frame 3:

	Acquire swapchain image (let's say index 2)
	Render G-Buffer using m_GBufferFramebuffers[0] (reusing frame in flight 0)
	Render Lighting using m_LightingFramebuffers[0] (reusing frame in flight 0)
	Render Composition to m_CompositionFramebuffers[2] (which uses swapchain image 2)
	Present swapchain image 2

	Frame 4:

	Acquire swapchain image (might be index 0 again if it's now available)
	Render G-Buffer using m_GBufferFramebuffers[1] (reusing frame in flight 1)
	Render Lighting using m_LightingFramebuffers[1] (reusing frame in flight 1)
	Render Composition to m_CompositionFramebuffers[0] (which uses swapchain image 0)
	Present swapchain image 0
 */
void VulkanDeferredRenderer::SubmitRenderPasses(uint32_t frameIndex)
{
	// Submit G-Buffer pass
	std::vector<VkSemaphore> gBufferWaitSemaphores = { m_Renderer->ImageAvailableSemaphore(frameIndex) };
	std::vector<VkPipelineStageFlags> gBufferWaitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkSemaphore> gBufferSignalSemaphores = { m_GBufferCompleteSemaphores[frameIndex] };
	std::vector<VulkanCommandBuffer*> gBufferCommandBuffers = { m_GBufferCommandBuffers[frameIndex].get() };

	VulkanCommandBuffer::Submit(
		VulkanContext::Get().GraphicsQueue(),
		gBufferCommandBuffers,
		gBufferWaitSemaphores,
		gBufferWaitStages,
		gBufferSignalSemaphores);

	// Submit Lighting pass
	std::vector<VkSemaphore> lightingWaitSemaphores = { m_GBufferCompleteSemaphores[frameIndex] };
	std::vector<VkPipelineStageFlags> lightingWaitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkSemaphore> lightingSignalSemaphores = { m_LightingCompleteSemaphores[frameIndex] };
	std::vector<VulkanCommandBuffer*> lightingCommandBuffers = { m_LightingCommandBuffers[frameIndex].get() };

	VulkanCommandBuffer::Submit(
		VulkanContext::Get().GraphicsQueue(),
		lightingCommandBuffers,
		lightingWaitSemaphores,
		lightingWaitStages,
		lightingSignalSemaphores);
}

void VulkanDeferredRenderer::RecordGBufferCommandBuffer(FrameInfo& frameInfo)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	auto frameIndex = frameInfo.FrameIndex;
	auto gBufferCmdBufferHandle = m_GBufferCommandBuffers[frameIndex]->GetHandle();

	m_GBufferCommandBuffers[frameIndex]->Begin();

	// Update internal host-side state to reflect the image transitions made during the render pass
	for (size_t i = 0; i < m_GBufferTextures[frameIndex].size() - 1; ++i)
		m_GBufferTextures[frameIndex][i]->UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRenderPassBeginInfo gBufferRenderPassInfo{};
	gBufferRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	gBufferRenderPassInfo.renderPass = m_GBufferPass->GetHandle();
	gBufferRenderPassInfo.framebuffer = m_GBufferFramebuffers[frameIndex]->GetHandle();
	gBufferRenderPassInfo.renderArea.offset = {0, 0};
	auto attachmentExtent = VkExtent2D{m_GBufferFramebuffers[frameIndex]->Width(), m_GBufferFramebuffers[frameIndex]->Height()};
	gBufferRenderPassInfo.renderArea.extent = attachmentExtent;

	m_GBufferPipeline->Bind(gBufferCmdBufferHandle);
	m_GBufferPass->BeginPass(gBufferCmdBufferHandle, gBufferRenderPassInfo, attachmentExtent);
	for (auto& [id, gameObject] : frameInfo.ActiveScene.GameObjects)
	{
		auto globalUbo = frameInfo.GlobalUbo.lock();
		gameObject.Render(gBufferCmdBufferHandle, frameIndex, globalUbo->DescriptorInfo());
	}
	m_GBufferPass->EndPass(gBufferCmdBufferHandle);

	// Update internal host-side state to reflect the image transitions made during the render pass
	for (size_t i = 0; i < m_GBufferTextures[frameIndex].size() - 1; ++i)
		m_GBufferTextures[frameIndex][i]->UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	m_GBufferCommandBuffers[frameIndex]->End();
}

void VulkanDeferredRenderer::RecordLightingPassCommandBuffer(FrameInfo& frameInfo)
{
	auto frameIndex = frameInfo.FrameIndex;

	auto lightingCmdBufferHandle = m_LightingCommandBuffers[frameIndex]->GetHandle();

	m_LightingCommandBuffers[frameIndex]->Begin();
	{
		m_LightingTextures[frameIndex]->UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderPassBeginInfo lightingRenderPassInfo{};
		lightingRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		lightingRenderPassInfo.renderPass = m_LightingPass->GetHandle();
		lightingRenderPassInfo.framebuffer = m_LightingFramebuffers[frameIndex]->GetHandle();
		lightingRenderPassInfo.renderArea.offset = {0, 0};
		auto attachmentExtent = VkExtent2D{m_LightingFramebuffers[frameIndex]->Width(), m_LightingFramebuffers[frameIndex]->Height()};
		lightingRenderPassInfo.renderArea.extent = attachmentExtent;

		m_LightingPipeline->Bind(lightingCmdBufferHandle);
		m_LightingPass->BeginPass(lightingCmdBufferHandle, lightingRenderPassInfo, attachmentExtent);
		{
			auto globalUbo = frameInfo.GlobalUbo.lock();
			m_LightingMaterial->UpdateDescriptorSets(frameIndex,
				{{0, {{.binding = 0, .type = DescriptorUpdate::Type::Buffer, .bufferInfo = globalUbo->DescriptorInfo()}}},
					{1,
						{{// Position
							 .binding = 0,
							 .type = DescriptorUpdate::Type::Image,
							 .imageInfo = m_GBufferTextures[frameIndex][0]->GetBaseViewDescriptorInfo()},
							{// Normal
								.binding = 1,
								.type = DescriptorUpdate::Type::Image,
								.imageInfo = m_GBufferTextures[frameIndex][1]->GetBaseViewDescriptorInfo()},
							{// Albedo
								.binding = 2,
								.type = DescriptorUpdate::Type::Image,
								.imageInfo = m_GBufferTextures[frameIndex][2]->GetBaseViewDescriptorInfo()}}}});
			m_LightingMaterial->SetPushConstant<int>("DebugDisplayIndex", 0);

			m_LightingMaterial->BindPushConstants(lightingCmdBufferHandle);
			m_LightingMaterial->BindDescriptors(frameIndex, lightingCmdBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS);
			vkCmdDraw(lightingCmdBufferHandle, 3, 1, 0, 0);
		}
		m_LightingPass->EndPass(lightingCmdBufferHandle);
		m_LightingTextures[frameIndex]->UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	m_LightingCommandBuffers[frameIndex]->End();
}

void VulkanDeferredRenderer::RecordCompositionPassCommandBuffer(FrameInfo& frameInfo)
{
	auto cmdBuffer = frameInfo.SwapchainSubmitCommandBuffer.lock();
	auto fbo = m_CompositionFramebuffers[frameInfo.ImageIndex]->GetHandle();

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	cmdBuffer->Begin();

	VkRenderPassBeginInfo compositionRenderPassInfo{};
	compositionRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	compositionRenderPassInfo.renderPass = m_CompositionPass->GetHandle();
	compositionRenderPassInfo.framebuffer = fbo;

	compositionRenderPassInfo.renderArea.offset = {0, 0};
	compositionRenderPassInfo.renderArea.extent = m_Renderer->VulkanSwapchain().Extent();

	m_CompositionPass->BeginPass(cmdBuffer->GetHandle(), compositionRenderPassInfo, m_Renderer->VulkanSwapchain().Extent());
	{
		m_CompositionPipeline->Bind(cmdBuffer->GetHandle());
		m_CompositionMaterial->UpdateDescriptorSets(
			frameInfo.FrameIndex,
			{
				{0,
					{
						{
							.binding = 0,
							.type = DescriptorUpdate::Type::Image,
							.imageInfo = m_LightingTextures[frameInfo.FrameIndex]->GetBaseViewDescriptorInfo()}
						}
					}
			});
		m_CompositionMaterial->BindDescriptors(frameInfo.FrameIndex, cmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS);
		vkCmdDraw(cmdBuffer->GetHandle(), 3, 1, 0, 0);
	}
	m_CompositionPass->EndPass(cmdBuffer->GetHandle());
	cmdBuffer->End();
}

void VulkanDeferredRenderer::OnEvent(Event& event)
{
}

void VulkanDeferredRenderer::Render(FrameInfo& frameInfo)
{
	VulkanCommandBuffer::ResetCommandBuffers(m_GBufferCommandBuffers);
	VulkanCommandBuffer::ResetCommandBuffers(m_LightingCommandBuffers);

	RecordGBufferCommandBuffer(frameInfo);
	RecordLightingPassCommandBuffer(frameInfo);
	RecordCompositionPassCommandBuffer(frameInfo);
	SubmitRenderPasses(frameInfo.FrameIndex);
}

void VulkanDeferredRenderer::Resize(uint32_t width, uint32_t height)
{
	for (int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		for (auto& gBufferTex : m_GBufferTextures[i])
			gBufferTex->Resize(width, height);

		m_LightingTextures[i]->Resize(width, height);
	}

	InvalidateGBufferPass();
	InvalidateLightingPass();
	InvalidateCompositionPass();
}

