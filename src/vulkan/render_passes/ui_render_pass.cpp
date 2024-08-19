#include "ui_render_pass.h"

#include "core/application.h"
#include "core/platform_path.h"
#include "render_graph.h"

#include <imgui.h>

void UIRenderPass::CreateResources(RenderGraph& graph)
{
	CreateCommandBuffers(graph);
	CreateSynchronizationPrimitives(graph);
	CreateShaders(graph);
	CreateMaterialLayout(graph);
	CreateMaterial(graph);
	CreateGraphicsPipeline(graph);
}

void UIRenderPass::Record(const FrameInfo& frameInfo, RenderGraph& graph)
{
	auto fontTextureResource = graph.GetResource<TextureResource>(m_FontTextureHandle);

	ResourceHandle<CommandBufferResource> cmdHandle = m_CommandBufferHandles[frameInfo.FrameIndex];
	auto* cmdBufferResource = graph.GetResource<CommandBufferResource>(cmdHandle);

	auto* fboResource = graph.GetResource<FramebufferResource>("Swapchain Framebuffer", frameInfo.FrameIndex);
	auto* renderPassResource = graph.GetResource<RenderPassObjectResource>("Swapchain Render Pass");
	auto* vertexBufferResource = graph.GetResource<BufferResource>("ImGui Vertex Buffer");
	auto* indexBufferResource = graph.GetResource<BufferResource>("ImGui Index Buffer");

	auto* graphicsPipelineResource = graph.GetResource<GraphicsPipelineObjectResource>(m_PipelineHandle);
	auto* materialResource = graph.GetResource<MaterialResource>(m_MaterialHandle);

	auto cmd = cmdBufferResource->Get();
	auto fbo = fboResource->Get();
	auto renderPass = renderPassResource->Get();
	auto pipeline = graphicsPipelineResource->Get();
	auto material = materialResource->Get();
	auto vbo = vertexBufferResource->Get();
	auto ebo = indexBufferResource->Get();

	cmd->Begin();

	VkRenderPassBeginInfo uiRenderPassBeginInfo{};
	uiRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	uiRenderPassBeginInfo.renderPass = renderPass->GetHandle();
	uiRenderPassBeginInfo.framebuffer = fbo->GetHandle();
	uiRenderPassBeginInfo.renderArea.offset = {0, 0};
	auto attachmentExtent = VkExtent2D{ fbo->Width(), fbo->Height() };
	uiRenderPassBeginInfo.renderArea.extent = attachmentExtent;

	renderPass->BeginPass(cmd->GetHandle(), uiRenderPassBeginInfo, attachmentExtent);
	{
		pipeline->Bind(cmd->GetHandle());
		m_TransformPushConstants.Scale = glm::vec2(2.0f / ImGui::GetIO().DisplaySize.x, 2.0f / ImGui::GetIO().DisplaySize.y);
		m_TransformPushConstants.Translate = glm::vec2(-1.0f);
		material->SetPushConstant<DisplayTransformPushConstants>("p_Transform", m_TransformPushConstants);

		material->UpdateDescriptorSets(
			frameInfo.FrameIndex,
			{
				{0,
					{
						{
							.binding = 0,
							.type = DescriptorUpdate::Type::Image,
							.imageInfo = fontTextureResource->Get()->GetBaseViewDescriptorInfo()
						}
					}
				}
			});

		material->BindPushConstants(cmd->GetHandle());
		material->BindDescriptors(frameInfo.FrameIndex, cmd->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS);

		// Render commands
		ImDrawData* imDrawData = ImGui::GetDrawData();
		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;

		if (imDrawData->CmdListsCount > 0)
		{
			VkDeviceSize offsets[1] = { 0 };
			VkBuffer buffers[] = {vbo->GetBuffer()};
			vkCmdBindVertexBuffers(cmd->GetHandle(), 0, 1, buffers, offsets);
			vkCmdBindIndexBuffer(cmd->GetHandle(), ebo->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

			for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
			{
				const ImDrawList* cmd_list = imDrawData->CmdLists[i];
				for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
				{
					const ImDrawCmd* imDrawCmd = &cmd_list->CmdBuffer[j];
					VkRect2D scissorRect;
					scissorRect.offset.x = std::max((int32_t)(imDrawCmd->ClipRect.x), 0);
					scissorRect.offset.y = std::max((int32_t)(imDrawCmd->ClipRect.y), 0);
					scissorRect.extent.width = (uint32_t)(imDrawCmd->ClipRect.z - imDrawCmd->ClipRect.x);
					scissorRect.extent.height = (uint32_t)(imDrawCmd->ClipRect.w - imDrawCmd->ClipRect.y);
					vkCmdSetScissor(cmd->GetHandle(), 0, 1, &scissorRect);
					vkCmdDrawIndexed(cmd->GetHandle(), imDrawCmd->ElemCount, 1, indexOffset, vertexOffset, 0);
					indexOffset += imDrawCmd->ElemCount;
				}
				vertexOffset += cmd_list->VtxBuffer.Size;
			}
		}

		renderPass->EndPass(cmd->GetHandle());
	}
	cmd->End();
}

void UIRenderPass::Submit(const FrameInfo& frameInfo, RenderGraph& graph)
{
	auto waitSemaphoreResource = graph.GetResource<SemaphoreResource>("Scene Composition Complete Semaphore", frameInfo.FrameIndex);
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

void UIRenderPass::CreateCommandBuffers(RenderGraph& graph)
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
		"ImGui Command Buffer",
		createCommandBuffer,
		context.GraphicsCommandPool(),
		true);
}

void UIRenderPass::CreateSynchronizationPrimitives(RenderGraph& graph)
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
		"ImGui Render Complete Semaphore",
		createSemaphore);
}

void UIRenderPass::CreateShaders(RenderGraph& graph)
{
	auto createShader =
		[](const std::string& resourceBaseName, const std::string& filePath, ShaderType shaderType)
	{
		auto shader = std::make_shared<VulkanShader>(filePath, shaderType);
		return std::make_shared<ShaderResource>(resourceBaseName, shader);
	};

	auto shaderDirectory = FileSystemUtil::GetShaderDirectory();
	auto uiVertPath = FileSystemUtil::PathToString(shaderDirectory / "ui.vert");
	auto uiFragPath = FileSystemUtil::PathToString(shaderDirectory / "ui.frag");
	m_VertexHandle = graph.CreateResource<ShaderResource>("ImGui Vertex Shader", createShader, uiVertPath, ShaderType::Vertex);
	m_FragmentHandle = graph.CreateResource<ShaderResource>("ImGui Fragment Shader", createShader, uiFragPath, ShaderType::Fragment);
}

void UIRenderPass::CreateMaterialLayout(RenderGraph& graph)
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
		"ImGui Material Layout",
		createMaterialLayout,
		*vertResource->Get(),
		*fragResource->Get());
}

void UIRenderPass::CreateMaterial(RenderGraph& graph)
{
	auto createMaterial =
		[](const std::string& resourceBaseName, const std::shared_ptr<VulkanMaterialLayout>& materialLayout)
	{
		auto material = std::make_shared<VulkanMaterial>(materialLayout);
		return std::make_shared<MaterialResource>(resourceBaseName, material);
	};

	auto materialLayoutResource = graph.GetResource<MaterialLayoutResource>(m_MaterialLayoutHandle);

	m_MaterialHandle = graph.CreateResource<MaterialResource>(
		"ImGui Material",
		createMaterial,
		materialLayoutResource->Get());
}

void UIRenderPass::CreateGraphicsPipeline(RenderGraph& graph)
{
	auto renderPassResource = graph.GetResource<RenderPassObjectResource>("Swapchain Render Pass");
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
		"ImGui Graphics Pipeline",
		createPipeline,
		renderPassResource->Get().get(),
		materialLayoutResource->Get().get(),
		vertShaderResource->Get().get(),
		fragShaderResource->Get().get());
}
