#include "ui_render_pass.h"

#include "core/application.h"
#include "core/platform_path.h"
#include "render_graph.h"
#include "render_graph_resource_declarations.h"
#include "render_pass_resources/all_resources.h"
#include "vulkan/vulkan_fence.h"
#include "vulkan/vulkan_semaphore.h"
#include "vulkan/vulkan_render_pass.h"
#include "vulkan/vulkan_framebuffer.h"

#include <imgui.h>

void UIRenderPass::CreateResources(RenderGraph& graph)
{
	CreateShaders(graph);
	CreateMaterialLayout(graph);
	CreateMaterial(graph);
	CreateGraphicsPipeline(graph);
}

void UIRenderPass::Record(const FrameInfo& frameInfo, RenderGraph& graph)
{
	VulkanCommandBuffer& cmd = graph.GetGlobalResourceHandle<CommandBufferResource>(SwapchainCommandBufferResourceName, frameInfo.FrameIndex)->Get();
	ResourceHandle<BufferResource> vertexBufferHandle = graph.GetGlobalResourceHandle<BufferResource>(UIVertexBufferResourceName, frameInfo.FrameIndex);
	ResourceHandle<BufferResource> indexBufferHandle = graph.GetGlobalResourceHandle<BufferResource>(UIIndexBufferResourceName, frameInfo.FrameIndex);
	VulkanGraphicsPipeline& pipeline = m_PipelineHandle->Get();
	VulkanMaterial& material = m_MaterialHandle->Get();

	VulkanBuffer* vbo = nullptr;
	VulkanBuffer* ebo = nullptr;

	ImDrawData* imDrawData = ImGui::GetDrawData();
	if (imDrawData->CmdListsCount > 0)
	{
		vbo = &vertexBufferHandle->Get();
		ebo = &indexBufferHandle->Get();
	}

	pipeline.Bind(cmd.GetHandle());
	m_TransformPushConstants.Scale = glm::vec2(2.0f / ImGui::GetIO().DisplaySize.x, 2.0f / ImGui::GetIO().DisplaySize.y);
	m_TransformPushConstants.Translate = glm::vec2(-1.0f);
	material.SetPushConstant<DisplayTransformPushConstants>("p_Transform", m_TransformPushConstants);
	material.BindPushConstants(cmd.GetHandle());

	// Render commands
	if (vbo && ebo)
	{
		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;

		VkDeviceSize offsets[1] = { 0 };
		VkBuffer buffers[] = {vbo->GetBuffer()};
		vkCmdBindVertexBuffers(cmd.GetHandle(), 0, 1, buffers, offsets);
		vkCmdBindIndexBuffer(cmd.GetHandle(), ebo->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

		VkDescriptorSet lastBoundDescriptorSet = VK_NULL_HANDLE;

		for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
		{
			const ImDrawList* cmd_list = imDrawData->CmdLists[i];
			for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
			{
				const ImDrawCmd* imDrawCmd = &cmd_list->CmdBuffer[j];

				if (imDrawCmd->TextureId && imDrawCmd->TextureId != VK_NULL_HANDLE)
				{
					auto currentDescriptorSet = static_cast<const VkDescriptorSet>(imDrawCmd->TextureId);
					if (currentDescriptorSet != lastBoundDescriptorSet)
					{
						vkCmdBindDescriptorSets(
							cmd.GetHandle(),
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							material.GetPipelineLayout(),
							0, 1,
							&currentDescriptorSet,
							0,
							nullptr);
						lastBoundDescriptorSet = currentDescriptorSet;
					}
				}

				// Set the scissor for the current draw command
				VkRect2D scissorRect;
				scissorRect.offset.x = std::max((int32_t)(imDrawCmd->ClipRect.x), 0);
				scissorRect.offset.y = std::max((int32_t)(imDrawCmd->ClipRect.y), 0);
				scissorRect.extent.width = (uint32_t)(imDrawCmd->ClipRect.z - imDrawCmd->ClipRect.x);
				scissorRect.extent.height = (uint32_t)(imDrawCmd->ClipRect.w - imDrawCmd->ClipRect.y);
				vkCmdSetScissor(cmd.GetHandle(), 0, 1, &scissorRect);

				// Issue the draw command
				vkCmdDrawIndexed(cmd.GetHandle(), imDrawCmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				indexOffset += imDrawCmd->ElemCount;
			}
			vertexOffset += cmd_list->VtxBuffer.Size;
		}
	}
}

void UIRenderPass::CreateShaders(RenderGraph& graph)
{
	auto createShader =
		[](size_t index, const std::string& name, const std::string& filePath, ShaderType shaderType)
	{
		auto shader = new VulkanShader(filePath, shaderType);
		return std::make_unique<ShaderResource>(name, shader);
	};

	auto shaderDirectory = FileSystemUtil::GetShaderDirectory();
	auto uiVertPath = FileSystemUtil::PathToString(shaderDirectory / "ui.vert");
	auto uiFragPath = FileSystemUtil::PathToString(shaderDirectory / "ui.frag");
	m_VertexHandle = graph.CreateResource<ShaderResource>(UIVertexShaderResourceName, createShader, uiVertPath, ShaderType::Vertex);
	m_FragmentHandle = graph.CreateResource<ShaderResource>(UIFragmentShaderResourceName, createShader, uiFragPath, ShaderType::Fragment);
}

void UIRenderPass::CreateMaterialLayout(RenderGraph& graph)
{
	auto createMaterialLayout =
		[](size_t index, const std::string& name, VulkanShader& vertShader, VulkanShader& fragShader)
	{
		auto materialLayout = new VulkanMaterialLayout(vertShader, fragShader, name);
		return std::make_unique<MaterialLayoutResource>(name, materialLayout);
	};

	ResourceHandle<ShaderResource> vertHandle = graph.GetGlobalResourceHandle<ShaderResource>(m_VertexHandle);
	ResourceHandle<ShaderResource> fragHandle = graph.GetGlobalResourceHandle<ShaderResource>(m_FragmentHandle);

	m_MaterialLayoutHandle = graph.CreateResource<MaterialLayoutResource>(
		UIMaterialLayoutResourceName,
		createMaterialLayout,
		vertHandle->Get(),
		fragHandle->Get());
}

void UIRenderPass::CreateMaterial(RenderGraph& graph)
{
	auto createMaterial =
		[](size_t index, const std::string& name, VulkanMaterialLayout& materialLayout)
	{
		auto material = new VulkanMaterial(materialLayout);
		return std::make_unique<MaterialResource>(name, material);
	};

	VulkanMaterialLayout& layoutRef = m_MaterialLayoutHandle->Get();

	m_MaterialHandle = graph.CreateResource<MaterialResource>(
		UIMaterialResourceName,
		createMaterial,
		layoutRef);
}

void UIRenderPass::CreateGraphicsPipeline(RenderGraph& graph)
{
	auto renderPassResourceHandle = graph.GetGlobalResourceHandle<RenderPassObjectResource>(SwapchainRenderPassResourceName);
	auto materialLayoutResourceHandle = graph.GetGlobalResourceHandle<MaterialLayoutResource>(m_MaterialLayoutHandle);
	auto& vertexShader = m_VertexHandle->Get();
	auto& fragmentShader = m_FragmentHandle->Get();

	auto createPipeline =
		[](	const std::string& resourceBaseName,
			VulkanRenderPass* renderPass,
			VulkanMaterialLayout* layout,
			VulkanShader* vertexShader,
			VulkanShader* fragmentShader)
	{
		auto bindingDescription = []()
		{
			std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
			bindingDescriptions[0].binding = 0;
			bindingDescriptions[0].stride = sizeof(ImDrawVert);
			bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescriptions;
		};

		auto attributeDescription = []()
		{
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

			attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)});
			attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)});
			attributeDescriptions.push_back({2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)});
			return attributeDescriptions;
		};

		auto builder =
			VulkanGraphicsPipelineBuilder(resourceBaseName)
				.SetShaders(*vertexShader, *fragmentShader)
				.SetVertexInputDescription({bindingDescription(), attributeDescription()})
				.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
				.SetPolygonMode(VK_POLYGON_MODE_FILL)
				.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
				.SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
				.SetDepthTesting(false, false, VK_COMPARE_OP_LESS_OR_EQUAL)
				.SetColorBlendAttachment(0, {
												.blendEnable = VK_TRUE,
												.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
												.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
												.colorBlendOp = VK_BLEND_OP_ADD,
												.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
												.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
												.alphaBlendOp = VK_BLEND_OP_ADD,
												.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
																  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
											})
			   .SetRenderPass(renderPass, 0)
			   .SetLayout(layout->GetPipelineLayout());

		auto pipeline = builder.Build();
		auto* rawPtr = pipeline.release();
		return std::make_unique<GraphicsPipelineResource>(resourceBaseName, rawPtr);
	};

	m_PipelineHandle = graph.CreateResource<GraphicsPipelineResource>(
		UIGraphicsPipelineResourceName,
		createPipeline,
		&renderPassResourceHandle->Get(),
		&materialLayoutResourceHandle->Get(),
		&vertexShader,
		&fragmentShader);
}

void UIRenderPass::OnSwapchainResize(uint32_t width, uint32_t height, RenderGraph& graph)
{
	graph.TryFreeResources<GraphicsPipelineObjectResource>(SceneCompositionGraphicsPipelineResourceName,  [](const std::shared_ptr<VulkanGraphicsPipeline>& graphicsPipeline){});
	CreateGraphicsPipeline(graph);
}
