#include "scene_composition_pass.h"

#include "core/application.h"
#include "core/platform_path.h"
#include "vulkan/vulkan_semaphore.h"
#include "vulkan/vulkan_fence.h"
#include "render_graph.h"

void SceneCompositionPass::CreateResources(RenderGraph& graph)
{
	CreateShaders(graph);
	CreateMaterialLayout(graph);
	CreateMaterial(graph);
	CreateGraphicsPipeline(graph);
}

void SceneCompositionPass::Record(const FrameInfo& frameInfo, RenderGraph& graph)
{
	auto cmdResource = graph.GetResource<CommandBufferResource>(SwapchainCommandBufferResourceName, frameInfo.FrameIndex);

	auto* lightingColorAttachment = graph.GetResource<TextureResource>(LightingColorAttachmentResourceName, frameInfo.FrameIndex);
	auto* graphicsPipelineResource = graph.GetResource<GraphicsPipelineObjectResource>(m_PipelineHandle);
	auto* materialResource = graph.GetResource<MaterialResource>(m_MaterialHandle);

	auto cmd = cmdResource->Get();
	auto pipeline = graphicsPipelineResource->Get();
	auto material = materialResource->Get();
	pipeline->Bind(cmd->GetHandle());
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
	auto swapchainRenderPassResource = graph.GetResource<RenderPassObjectResource>(SwapchainRenderPassResourceName);
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
		swapchainRenderPassResource->Get().get(),
		materialLayoutResource->Get().get(),
		vertShaderResource->Get().get(),
		fragShaderResource->Get().get());
}
