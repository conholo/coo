#include "vulkan_deferred_renderer.h"

#include "vulkan_model.h"
#include "vulkan_renderer.h"
#include "vulkan_utils.h"

#include <vulkan/vulkan.h>

VulkanDeferredRenderer::VulkanDeferredRenderer(VulkanRenderer* renderer) : m_Renderer(renderer)
{
}

VulkanDeferredRenderer::~VulkanDeferredRenderer()
{
	vkFreeCommandBuffers(VulkanContext::Get().Device(), VulkanContext::Get().GraphicsCommandPool(), m_GBufferCommandBuffers.size(),
		m_GBufferCommandBuffers.data());
	vkFreeCommandBuffers(VulkanContext::Get().Device(), VulkanContext::Get().GraphicsCommandPool(), m_LightingCommandBuffers.size(),
		m_LightingCommandBuffers.data());

	for (auto semaphore : m_GBufferCompleteSemaphores)
		vkDestroySemaphore(VulkanContext::Get().Device(), semaphore, nullptr);
	for (auto semaphore : m_LightingCompleteSemaphores)
		vkDestroySemaphore(VulkanContext::Get().Device(), semaphore, nullptr);
}

void VulkanDeferredRenderer::Initialize()
{
	CreateCommandBuffers();
	CreateSynchronizationPrimitives();

	m_FullScreenQuadVertexShader = std::make_shared<VulkanShader>("../assets/shaders/fsq.vert", ShaderType::Vertex);
	CreateGBufferResources();
	CreateLightingResources();
	CreateCompositionResources();
}

void VulkanDeferredRenderer::Shutdown()
{
}

void VulkanDeferredRenderer::CreateCommandBuffers()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = VulkanContext::Get().GraphicsCommandPool();
	allocInfo.commandBufferCount = VulkanSwapchain::MAX_FRAMES_IN_FLIGHT;

	m_GBufferCommandBuffers.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	m_LightingCommandBuffers.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);

	vkAllocateCommandBuffers(VulkanContext::Get().Device(), &allocInfo, m_GBufferCommandBuffers.data());
	vkAllocateCommandBuffers(VulkanContext::Get().Device(), &allocInfo, m_LightingCommandBuffers.data());
}

void VulkanDeferredRenderer::CreateSynchronizationPrimitives()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	m_GBufferCompleteSemaphores.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	m_LightingCompleteSemaphores.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);
	m_CompositionRenderCompleteSemaphores.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	m_GBufferCompleteFence.resize(VulkanSwapchain::MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkCreateSemaphore(VulkanContext::Get().Device(), &semaphoreInfo, nullptr, &m_GBufferCompleteSemaphores[i]);
		vkCreateSemaphore(VulkanContext::Get().Device(), &semaphoreInfo, nullptr, &m_LightingCompleteSemaphores[i]);
		vkCreateSemaphore(VulkanContext::Get().Device(), &semaphoreInfo, nullptr, &m_CompositionRenderCompleteSemaphores[i]);
		VK_CHECK_RESULT(vkCreateFence(VulkanContext::Get().Device(), &fenceInfo, nullptr, &m_GBufferCompleteFence[i]));
	}
}

void VulkanDeferredRenderer::CreateGBufferResources()
{
	CreateGBufferTextures();
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

	// Dependency from outside of pass to inside G-Buffer pass where it writes to the attachments.
	m_GBufferPass->AddDependency(
		VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT ,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_DEPENDENCY_BY_REGION_BIT);

	// Dependency from inside G-Buffer pass to outside the pass (lighting pass) where it will read from the g-buffer attachments.
	m_GBufferPass->AddDependency(
		0, VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_DEPENDENCY_BY_REGION_BIT);

	m_GBufferPass->Build();
}

void VulkanDeferredRenderer::CreateGBufferPipeline()
{
	if(!m_GBufferVertexShader || !m_GBufferFragmentShader)
	{
		m_GBufferVertexShader = std::make_shared<VulkanShader>("../assets/shaders/gbuffer.vert", ShaderType::Vertex);
		m_GBufferFragmentShader = std::make_shared<VulkanShader>("../assets/shaders/gbuffer.frag", ShaderType::Fragment);
	}

	m_GBufferMaterialLayout = std::make_shared<VulkanMaterialLayout>(m_GBufferVertexShader, m_GBufferFragmentShader);
	m_GBufferBaseMaterial = std::make_shared<VulkanMaterial>(m_GBufferMaterialLayout);

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

		m_GBufferFramebuffers[i] = std::make_unique<VulkanFramebuffer>("G-Buffer Framebuffer " + std::to_string(i));
		m_GBufferFramebuffers[i]->Create(m_GBufferPass->RenderPass(), attachments, extent.width, extent.height);
	}
}

/*
 * Lighting
 */
void VulkanDeferredRenderer::CreateLightingResources()
{
	CreateLightingTextures();
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
	m_LightingPass->AddAttachment({
		.Type = AttachmentType::Color,
		.Format = ImageFormat::RGBA,
		.Samples = VK_SAMPLE_COUNT_1_BIT,
		.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
		.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.FinalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.ClearValue{.color = {1.0f, 0.0f, 0.0f, 1.0f}}
	});

	// Set up subpass
	SubpassDescription subpass;
	subpass.ColorAttachments = {0};	   // Color
	m_LightingPass->AddSubpass(subpass);

	// From external pass (G-Buffer) to lighting pass
	m_LightingPass->AddDependency(
		VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_DEPENDENCY_BY_REGION_BIT);

	// From lighting pass to external pass (composition)
	m_LightingPass->AddDependency(0, VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_DEPENDENCY_BY_REGION_BIT);

	m_LightingPass->Build();
}

void VulkanDeferredRenderer::CreateLightingPipeline()
{
	if(!m_LightingFragmentShader)
		m_LightingFragmentShader = std::make_shared<VulkanShader>("../assets/shaders/lighting.frag", ShaderType::Fragment);
	if(!m_LightingMaterialLayout)
		m_LightingMaterialLayout = std::make_shared<VulkanMaterialLayout>(m_FullScreenQuadVertexShader, m_LightingFragmentShader);
	if(!m_LightingMaterial)
		m_LightingMaterial = std::make_shared<VulkanMaterial>(m_LightingMaterialLayout);

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
		m_LightingFramebuffers[i] = std::make_unique<VulkanFramebuffer>("Lighting Framebuffer " + std::to_string(i));
		m_LightingFramebuffers[i]->Create(m_LightingPass->RenderPass(), attachments, extent.width, extent.height);
	}
}

void VulkanDeferredRenderer::CreateCompositionResources()
{
	CreateCompositionRenderPass();
	CreateCompositionPipeline();
	CreateCompositionFramebuffers();
}

void VulkanDeferredRenderer::CreateCompositionRenderPass()
{
	m_CompositionPass = std::make_unique<VulkanRenderPass>("Composition Render Pass");

	// Final color attachment (swapchain image)
	m_CompositionPass->AddAttachment({.Type = AttachmentType::Color,
		.Format = ImageUtils::VulkanFormatToImageFormat(m_Renderer->VulkanSwapchain().SwapchainImageFormat()),
		.Samples = VK_SAMPLE_COUNT_1_BIT,
		.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.StoreOp = VK_ATTACHMENT_STORE_OP_STORE,
		.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.FinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.ClearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}});

	// Set up subpass
	SubpassDescription subpass;
	subpass.ColorAttachments = {0};	   // Final color output
	m_CompositionPass->AddSubpass(subpass);

	// Dependency from Lighting pass to Composition pass
	m_CompositionPass->AddDependency(VK_SUBPASS_EXTERNAL, 0,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,	  	// Wait for lighting pass to *finish*,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,			  	// then the composition pass can *start*.
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,			  	// Once the lighting pass has *written* to its attachments,
		VK_ACCESS_SHADER_READ_BIT,						// Then the composition pass can *read* the attachment in the shader.
		VK_DEPENDENCY_BY_REGION_BIT);

	// Dependency from Composition pass to final output
	m_CompositionPass->AddDependency(0, VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,	  	// Wait for the composition writes to *finish*,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,			  	// then the frame writing is complete and then display can *start*,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,			  	// Once the composition pass has *written* to its attachment,
		VK_ACCESS_MEMORY_READ_BIT,	  					// Then the impl. can *read* the memory of the attachment (swapchain image).
		VK_DEPENDENCY_BY_REGION_BIT);

	m_CompositionPass->Build();
}

void VulkanDeferredRenderer::CreateCompositionPipeline()
{
	if(!m_CompositionFragmentShader)
		m_CompositionFragmentShader = std::make_shared<VulkanShader>("../assets/shaders/texture_display.frag", ShaderType::Fragment);

	m_CompositionMaterialLayout = std::make_shared<VulkanMaterialLayout>(m_FullScreenQuadVertexShader, m_CompositionFragmentShader);
	m_CompositionMaterial = std::make_shared<VulkanMaterial>(m_CompositionMaterialLayout);

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
	VkExtent2D extent = m_Renderer->VulkanSwapchain().Extent();
	uint32_t imageCount = m_Renderer->VulkanSwapchain().ImageCount();

	m_CompositionFramebuffers.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; i++)
	{
		auto weakImage = m_Renderer->VulkanSwapchain().GetImage(i);
		auto sharedImage = weakImage.lock();

		std::vector<VkImageView> attachments;
		if (sharedImage)
			attachments.push_back(sharedImage->GetView()->GetImageView());

		m_CompositionFramebuffers[i] = std::make_unique<VulkanFramebuffer>("Composition Framebuffer " + std::to_string(i));
		m_CompositionFramebuffers[i]->Create(m_CompositionPass->RenderPass(), attachments, extent.width, extent.height);
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
	VkSubmitInfo gBufferSubmitInfo{};
	gBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	std::vector<VkSemaphore> gBufferWaitSemaphores = { m_Renderer->ImageAvailableSemaphore(frameIndex) };
	VkPipelineStageFlags gBufferWaitStages[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT }; // <- We're saying "don't start writing to color attachments until the previous operation is complete
	std::vector<VkSemaphore> gBufferSignalSemaphores = { m_GBufferCompleteSemaphores[frameIndex] };

	gBufferSubmitInfo.waitSemaphoreCount = static_cast<uint32_t>(gBufferWaitSemaphores.size());
	gBufferSubmitInfo.pWaitSemaphores = gBufferWaitSemaphores.data();
	gBufferSubmitInfo.pWaitDstStageMask = gBufferWaitStages;

	gBufferSubmitInfo.signalSemaphoreCount = static_cast<uint32_t>(gBufferSignalSemaphores.size());
	gBufferSubmitInfo.pSignalSemaphores = gBufferSignalSemaphores.data();

	gBufferSubmitInfo.commandBufferCount = 1;
	gBufferSubmitInfo.pCommandBuffers = &m_GBufferCommandBuffers[frameIndex];

	vkQueueSubmit(VulkanContext::Get().GraphicsQueue(), 1, &gBufferSubmitInfo, VK_NULL_HANDLE);

	// Submit Lighting pass
	VkSubmitInfo lightingSubmitInfo{};
	lightingSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	std::vector<VkSemaphore> lightingWaitSemaphores = { m_GBufferCompleteSemaphores[frameIndex] };
	VkPipelineStageFlags lightingWaitStages[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT }; // <- We're saying "don't start writing to color attachments until the previous operation is complete
	std::vector<VkSemaphore> lightingSignalSemaphores = { m_LightingCompleteSemaphores[frameIndex] };

	lightingSubmitInfo.waitSemaphoreCount = static_cast<uint32_t>(lightingWaitSemaphores.size());
	lightingSubmitInfo.pWaitSemaphores = lightingWaitSemaphores.data();
	lightingSubmitInfo.pWaitDstStageMask = lightingWaitStages;

	lightingSubmitInfo.signalSemaphoreCount = static_cast<uint32_t>(lightingSignalSemaphores.size());
	lightingSubmitInfo.pSignalSemaphores = lightingSignalSemaphores.data();

	lightingSubmitInfo.commandBufferCount = 1;
	lightingSubmitInfo.pCommandBuffers = &m_LightingCommandBuffers[frameIndex];

	vkQueueSubmit(VulkanContext::Get().GraphicsQueue(), 1, &lightingSubmitInfo, VK_NULL_HANDLE);

	/*
	 * Composition Pass will be submitted by the Swapchain Renderer.
	 */
}

void VulkanDeferredRenderer::RecordGBufferCommandBuffer(FrameInfo& frameInfo)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	auto frameIndex = frameInfo.FrameIndex;
	auto gBufferCmd = m_GBufferCommandBuffers[frameIndex];

	vkBeginCommandBuffer(gBufferCmd, &beginInfo);

	VkRenderPassBeginInfo gBufferRenderPassInfo{};
	gBufferRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	gBufferRenderPassInfo.renderPass = m_GBufferPass->RenderPass();
	gBufferRenderPassInfo.framebuffer = m_GBufferFramebuffers[frameIndex]->Framebuffer();
	gBufferRenderPassInfo.renderArea.offset = {0, 0};
	auto attachmentExtent = VkExtent2D{ m_GBufferFramebuffers[frameIndex]->Width(), m_GBufferFramebuffers[frameIndex]->Height() };
	gBufferRenderPassInfo.renderArea.extent = attachmentExtent;

	m_GBufferPipeline->Bind(gBufferCmd);
	m_GBufferPass->BeginPass(gBufferCmd, gBufferRenderPassInfo, attachmentExtent);
	for (auto& [id, gameObject] : frameInfo.ActiveScene.GameObjects)
	{
		gameObject.Render(gBufferCmd, frameIndex, frameInfo.GlobalUbo->DescriptorInfo());
	}
	m_GBufferPass->EndPass(gBufferCmd);

	// Update internal host-side state to reflect the image transitions made during the render pass
	for (size_t i = 0; i < m_GBufferTextures[frameIndex].size() - 1; ++i)
		m_GBufferTextures[frameIndex][i]->UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkEndCommandBuffer(gBufferCmd);
}

void VulkanDeferredRenderer::RecordLightingPassCommandBuffer(FrameInfo& frameInfo)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	auto frameIndex = frameInfo.FrameIndex;
	auto lightingCmd = m_LightingCommandBuffers[frameIndex];

	vkBeginCommandBuffer(lightingCmd, &beginInfo);
	{
		VkRenderPassBeginInfo lightingRenderPassInfo{};
		lightingRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		lightingRenderPassInfo.renderPass = m_LightingPass->RenderPass();
		lightingRenderPassInfo.framebuffer = m_LightingFramebuffers[frameIndex]->Framebuffer();
		lightingRenderPassInfo.renderArea.offset = {0, 0};
		auto attachmentExtent = VkExtent2D{ m_LightingFramebuffers[frameIndex]->Width(), m_LightingFramebuffers[frameIndex]->Height() };
		lightingRenderPassInfo.renderArea.extent = attachmentExtent;

		m_LightingPipeline->Bind(lightingCmd);
		m_LightingPass->BeginPass(lightingCmd, lightingRenderPassInfo, attachmentExtent);
		{
			m_LightingMaterial->UpdateDescriptorSets(frameIndex,
				{
					{0,
						{
							{
								.binding = 0,
								.type = DescriptorUpdate::Type::Buffer,
								.bufferInfo = frameInfo.GlobalUbo->DescriptorInfo()
							}
						}
					},
					{1,
						{
							{// Position
								.binding = 0,
								.type = DescriptorUpdate::Type::Image,
								.imageInfo = m_GBufferTextures[frameIndex][0]->GetBaseViewDescriptorInfo()
							},
							{// Normal
								.binding = 1,
								.type = DescriptorUpdate::Type::Image,
								.imageInfo = m_GBufferTextures[frameIndex][1]->GetBaseViewDescriptorInfo()
							},
							{// Albedo
								.binding = 2,
								.type = DescriptorUpdate::Type::Image,
								.imageInfo = m_GBufferTextures[frameIndex][2]->GetBaseViewDescriptorInfo()}
						}
					}
				});

			m_LightingMaterial->BindDescriptors(frameIndex, lightingCmd, VK_PIPELINE_BIND_POINT_GRAPHICS);
			vkCmdDraw(lightingCmd, 3, 1, 0, 0);
		}
		m_LightingPass->EndPass(lightingCmd);
		// Update internal host-side state to reflect the image transitions made during the render pass
		m_LightingTextures[frameIndex]->UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	vkEndCommandBuffer(lightingCmd);
}

void VulkanDeferredRenderer::RecordCompositionPassCommandBuffer(FrameInfo& frameInfo)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	vkBeginCommandBuffer(frameInfo.DrawCommandBuffer, &beginInfo);

	uint32_t swapchainImageIndex = m_Renderer->GetCurrentSwapchainImageIndex();
	VkRenderPassBeginInfo compositionRenderPassInfo{};
	compositionRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	compositionRenderPassInfo.renderPass = m_CompositionPass->RenderPass();
	compositionRenderPassInfo.framebuffer = m_CompositionFramebuffers[swapchainImageIndex]->Framebuffer();

	compositionRenderPassInfo.renderArea.offset = {0, 0};
	compositionRenderPassInfo.renderArea.extent = m_Renderer->VulkanSwapchain().Extent();

	m_CompositionPass->BeginPass(frameInfo.DrawCommandBuffer, compositionRenderPassInfo, m_Renderer->VulkanSwapchain().Extent());
	m_CompositionPipeline->Bind(frameInfo.DrawCommandBuffer);
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
	m_CompositionMaterial->BindDescriptors(frameInfo.FrameIndex, frameInfo.DrawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
	vkCmdDraw(frameInfo.DrawCommandBuffer, 3, 1, 0, 0);
	m_CompositionPass->EndPass(frameInfo.DrawCommandBuffer);

	vkEndCommandBuffer(frameInfo.DrawCommandBuffer);
}

void VulkanDeferredRenderer::Render(FrameInfo& frameInfo)
{
	RecordGBufferCommandBuffer(frameInfo);
	RecordLightingPassCommandBuffer(frameInfo);
	RecordCompositionPassCommandBuffer(frameInfo);

	SubmitRenderPasses(frameInfo.FrameIndex);
	frameInfo.WaitForGraphicsSubmit = m_LightingCompleteSemaphores[frameInfo.FrameIndex];
	frameInfo.SignalForPresentation = m_CompositionRenderCompleteSemaphores[frameInfo.FrameIndex];
}

void VulkanDeferredRenderer::Resize(uint32_t width, uint32_t height)
{
	for (int i = 0; i < VulkanSwapchain::MAX_FRAMES_IN_FLIGHT; ++i)
	{
		for (auto& gBufferTex : m_GBufferTextures[i])
			gBufferTex->Resize(width, height);

		m_LightingTextures[i]->Resize(width, height);
	}

	m_GBufferPass.reset();
	m_GBufferPipeline.reset();
	m_GBufferFramebuffers.clear();
	m_GBufferMaterialLayout.reset();
	m_GBufferBaseMaterial.reset();
	CreateGBufferResources();

	m_LightingPass.reset();
	m_LightingPipeline.reset();
	m_LightingFramebuffers.clear();
	m_LightingMaterialLayout.reset();
	m_LightingMaterial.reset();
	CreateLightingResources();

	m_CompositionPass.reset();
	m_CompositionPipeline.reset();
	m_CompositionFramebuffers.clear();
	m_CompositionMaterialLayout.reset();
	m_CompositionMaterial.reset();
	CreateCompositionResources();
}

void VulkanDeferredRenderer::RegisterGameObject(GameObject& gameObjectRef)
{
	gameObjectRef.Material = m_GBufferBaseMaterial->Clone();
}
