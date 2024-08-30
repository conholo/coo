#include "lighting_render_pass.h"
#include "render_graph.h"
#include "core/application.h"
#include "core/platform_path.h"
#include "render_graph_resource_declarations.h"
#include "vulkan/vulkan_fence.h"
#include "vulkan/vulkan_semaphore.h"
#include "vulkan/vulkan_render_pass.h"
#include "vulkan/vulkan_framebuffer.h"
#include "vulkan/render_passes/render_pass_resources/buffer_resource.h"

void LightingPass::CreateResources(RenderGraph& graph)
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

void LightingPass::Record(const FrameInfo& frameInfo, RenderGraph& graph)
{
	auto& colorAttachment = m_ColorAttachmentHandles[frameInfo.FrameIndex].Get()->Get();
	// Update internal host-side state to reflect the image transitions made during the render pass
	colorAttachment.UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	auto gBufferPositionHandle = graph.GetResourceHandle<TextureResource>(
		GBufferPositionAttachmentTextureResourceName,
		m_uuid,
		frameInfo.FrameIndex);
	auto gBufferNormalHandle = graph.GetResourceHandle<TextureResource>(
		GBufferNormalAttachmentTextureResourceName,
		m_uuid,
		frameInfo.FrameIndex);
	auto gBufferAlbedoHandle = graph.GetResourceHandle<TextureResource>(
		GBufferAlbedoAttachmentTextureResourceName,
		m_uuid,
		frameInfo.FrameIndex);

	VulkanCommandBuffer& cmd = m_CommandBufferHandles[frameInfo.FrameIndex]->Get();
	VulkanFramebuffer& fbo = m_FramebufferHandles[frameInfo.FrameIndex].Get()->Get();
	VulkanRenderPass& renderPass = m_RenderPassHandle->Get();
	VulkanGraphicsPipeline& pipeline = m_PipelineHandle->Get();
	VulkanMaterial& material = m_MaterialHandle->Get();
	VulkanFence& resourceFence = m_ResourcesInFlightFenceHandles[frameInfo.FrameIndex]->Get();

	cmd.WaitForCompletion(resourceFence.GetHandle());

	cmd.Begin();
	{
		colorAttachment.UpdateState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderPassBeginInfo lightingRenderPassInfo{};
		lightingRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		lightingRenderPassInfo.renderPass = renderPass.GetHandle();
		lightingRenderPassInfo.framebuffer = fbo.GetHandle();
		lightingRenderPassInfo.renderArea.offset = {0, 0};
		auto attachmentExtent = VkExtent2D{ fbo.Width(), fbo.Height() };
		lightingRenderPassInfo.renderArea.extent = attachmentExtent;

		pipeline.Bind(cmd.GetHandle());
		renderPass.BeginPass(cmd.GetHandle(), lightingRenderPassInfo, attachmentExtent);
		{
			auto globalUboResource = graph.GetGlobalResourceHandle<BufferResource>(GlobalUniformBufferResourceName, frameInfo.FrameIndex);
			material.UpdateDescriptorSets(frameInfo.FrameIndex,
				{{0, {{.binding = 0, .type = DescriptorUpdate::Type::Buffer, .bufferInfo = globalUboResource->Get().DescriptorInfo()}}},
					{1,
						{{// Position
							 .binding = 0,
							 .type = DescriptorUpdate::Type::Image,
							 .imageInfo = gBufferPositionHandle->Get().GetBaseViewDescriptorInfo()},
							{// Normal
								.binding = 1,
								.type = DescriptorUpdate::Type::Image,
								.imageInfo = gBufferNormalHandle->Get().GetBaseViewDescriptorInfo()},
							{// Albedo
								.binding = 2,
								.type = DescriptorUpdate::Type::Image,
								.imageInfo = gBufferAlbedoHandle->Get().GetBaseViewDescriptorInfo()}}}});

			material.SetPushConstant<int>("DebugDisplayIndex", 0);
			material.BindPushConstants(cmd.GetHandle());
			material.BindDescriptors(frameInfo.FrameIndex, cmd.GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS);
			vkCmdDraw(cmd.GetHandle(), 3, 1, 0, 0);
		}
		renderPass.EndPass(cmd.GetHandle());
		colorAttachment.UpdateState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	cmd.End();
}

void LightingPass::Submit(const FrameInfo& frameInfo, RenderGraph& graph)
{
	auto waitSemaphoreResource = graph.GetGlobalResourceHandle<SemaphoreResource>(GBufferRenderCompleteSemaphoreResourceName, frameInfo.FrameIndex);
	auto signalSemaphoreResource = m_RenderCompleteSemaphoreHandles[frameInfo.FrameIndex].Get();
	auto cmdBufferResource = m_CommandBufferHandles[frameInfo.FrameIndex].Get();

	std::vector<VkSemaphore> waitSemaphores = {waitSemaphoreResource->Get().GetHandle() };
	std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::vector<VkSemaphore> signalSemaphores = {signalSemaphoreResource->Get().GetHandle() };
	std::vector<VulkanCommandBuffer*> cmdBuffers = { &cmdBufferResource->Get() };
	auto fenceResource = m_ResourcesInFlightFenceHandles[frameInfo.FrameIndex].Get();

	VulkanCommandBuffer::Submit(
		VulkanContext::Get().GraphicsQueue(),
		cmdBuffers,
		waitSemaphores,
		waitStages,
		signalSemaphores,
		fenceResource->Get().GetHandle());
}

void LightingPass::CreateCommandBuffers(RenderGraph& graph)
{
	auto& context = VulkanContext::Get();

	auto createCommandBuffer =
		[](size_t index, const std::string& name, VkCommandPool pool, bool isPrimary)
	{
		auto* commandBuffer = new VulkanCommandBuffer(pool, isPrimary, name);
		return std::make_unique<CommandBufferResource>(name, commandBuffer);
	};

	m_CommandBufferHandles = graph.CreateResources<CommandBufferResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		LightingCommandBufferResourceName,
		createCommandBuffer,
		context.GraphicsCommandPool(),
		true);
}

void LightingPass::CreateSynchronizationPrimitives(RenderGraph& graph)
{
	auto createSemaphore = [](size_t index, const std::string& name)
	{
		auto semaphore = new VulkanSemaphore(name);
		return std::make_unique<SemaphoreResource>(name, semaphore);
	};

	m_RenderCompleteSemaphoreHandles = graph.CreateResources<SemaphoreResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		LightingRenderCompleteSemaphoreResourceName,
		createSemaphore);

	auto createFence = [](size_t index, const std::string& name)
	{
		auto fence = new VulkanFence(name, true);
		return std::make_unique<FenceResource>(name, fence);
	};

	m_ResourcesInFlightFenceHandles = graph.CreateResources<FenceResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		LightingResourcesInFlightResourceName,
		createFence);
}

void LightingPass::CreateTextures(RenderGraph& graph)
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
		auto* rawPtr = texture.release();
		return std::make_unique<TextureResource>(byteAttachmentSpec.DebugName, rawPtr);
	};

	m_ColorAttachmentHandles = graph.CreateResources<TextureResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		LightingColorAttachmentResourceName,
		createColorTexture);
}

void LightingPass::CreateRenderPass(RenderGraph& graph)
{
	auto createRenderPass =
		[](size_t index, const std::string& name)
	{
		auto renderPass = new VulkanRenderPass(name);

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
		return std::make_unique<RenderPassObjectResource>(name, renderPass);
	};

	m_RenderPassHandle = graph.CreateResource<RenderPassObjectResource>(LightingRenderPassResourceName, createRenderPass);
}

void LightingPass::CreateShaders(RenderGraph& graph)
{
	auto createShader =
		[](size_t index, const std::string& name, const std::string& filePath, ShaderType shaderType)
	{
		auto shader = new VulkanShader(filePath, shaderType);
		return std::make_unique<ShaderResource>(name, shader);
	};

	auto shaderDirectory = FileSystemUtil::GetShaderDirectory();
	auto lightingFragPath = FileSystemUtil::PathToString(shaderDirectory / "lighting.frag");
	m_FragmentHandle = graph.CreateResource<ShaderResource>(LightingFragmentShaderResourceName, createShader, lightingFragPath, ShaderType::Fragment);
}

void LightingPass::CreateMaterialLayout(RenderGraph& graph)
{
	auto createMaterialLayout =
		[](size_t index, const std::string& name, VulkanShader& vertShader, VulkanShader& fragShader)
	{
		auto materialLayout = new VulkanMaterialLayout(vertShader, fragShader, name);
		return std::make_unique<MaterialLayoutResource>(name, materialLayout);
	};

	auto vertResource = graph.GetGlobalResourceHandle<ShaderResource>(FullScreenQuadShaderResourceName);
	VulkanShader& vertShader = vertResource->Get();
	VulkanShader& fragShader = m_FragmentHandle.Get()->Get();

	m_MaterialLayoutHandle = graph.CreateResource<MaterialLayoutResource>(
		LightingMaterialLayoutResourceName,
		createMaterialLayout,
		vertShader,
		fragShader);
}

void LightingPass::CreateMaterial(RenderGraph& graph)
{
	auto createMaterial =
		[](size_t index, const std::string& name, VulkanMaterialLayout& materialLayoutRef)
	{
		auto material = new VulkanMaterial(materialLayoutRef);
		return std::make_unique<MaterialResource>(name, material);
	};

	auto& materialLayoutResource = m_MaterialLayoutHandle.Get()->Get();

	m_MaterialHandle = graph.CreateResource<MaterialResource>(
		LightingMaterialResourceName,
		createMaterial,
		materialLayoutResource);
}

void LightingPass::CreateGraphicsPipeline(RenderGraph& graph)
{
	auto* renderPass = &m_RenderPassHandle.Get()->Get();
	auto* materialLayout = &m_MaterialLayoutHandle.Get()->Get();
	auto vertResource = graph.GetGlobalResourceHandle<ShaderResource>(FullScreenQuadShaderResourceName);
	VulkanShader* vertShader = &vertResource->Get();
	auto* fragShader = &m_FragmentHandle.Get()->Get();

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
		LightingGraphicsPipelineResourceName,
		createPipeline,
		renderPass,
		materialLayout,
		vertShader,
		fragShader);
}

void LightingPass::CreateFramebuffers(RenderGraph& graph)
{
	auto& swapchain = Application::Get().GetRenderer().VulkanSwapchain();
	VkExtent2D extent = swapchain.Extent();

	auto createFramebuffer =
		[&extent, this](
			size_t index,
			const std::string& name,
			VulkanRenderPass* renderPass,
			const std::vector<ResourceHandle<TextureResource>>& colorTextureHandles)
	{
		auto colorView = m_ColorAttachmentHandles[index].Get()->Get().GetImage()->GetView();

		std::vector<VkImageView> attachments = {
			colorView->GetImageView()
		};

		auto framebuffer = new VulkanFramebuffer(name);
		framebuffer->Create(renderPass->GetHandle(), attachments, extent.width, extent.height);
		return std::make_unique<FramebufferResource>(name, framebuffer);
	};

	auto* renderPass = &m_RenderPassHandle.Get()->Get();

	m_FramebufferHandles = graph.CreateResources<FramebufferResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		LightingFramebufferResourceName,
		createFramebuffer,
		renderPass,
		m_ColorAttachmentHandles);
}

void LightingPass::OnSwapchainResize(uint32_t width, uint32_t height, RenderGraph& graph)
{
	for (auto cmdBufferHandle : m_CommandBufferHandles)
	{
		auto cmdResource = cmdBufferHandle.Get();

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(VulkanContext::Get().Device(), &fenceCreateInfo, nullptr, &fence));
		VK_CHECK_RESULT(cmdResource->Get().InterruptAndReset(fence, true));
		vkDestroyFence(VulkanContext::Get().Device(), fence, nullptr);
	}

	auto freeFences = [](const std::shared_ptr<VulkanFence>& fence){};
	graph.TryFreeResources<FenceResource>(LightingResourcesInFlightResourceName, freeFences);
	auto freeSemaphores = [](const std::shared_ptr<VulkanSemaphore>& semaphore){};
	graph.TryFreeResources<SemaphoreResource>(LightingRenderCompleteSemaphoreResourceName, freeSemaphores);
	CreateSynchronizationPrimitives(graph);

	for(auto colorHandle : m_ColorAttachmentHandles)
		colorHandle.Get()->Get().Resize(width, height);

	graph.TryFreeResources<RenderPassObjectResource>(LightingRenderPassResourceName,  [](const std::shared_ptr<VulkanRenderPass>& renderPass){});
	CreateRenderPass(graph);

	graph.TryFreeResources<GraphicsPipelineObjectResource>(LightingGraphicsPipelineResourceName,  [](const std::shared_ptr<VulkanGraphicsPipeline>& graphicsPipeline){});
	CreateGraphicsPipeline(graph);

	graph.TryFreeResources<FramebufferResource>(LightingFramebufferResourceName,  [](const std::shared_ptr<VulkanFramebuffer>& framebuffer){});
	CreateFramebuffers(graph);
}