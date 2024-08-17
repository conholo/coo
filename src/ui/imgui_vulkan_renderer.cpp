#include "vulkan/vulkan_swapchain_renderer.h"

#include "ui/imgui_vulkan_renderer.h"
#include "vulkan/vulkan_render_pass.h"
#include "vulkan/vulkan_framebuffer.h"
#include "vulkan/vulkan_graphics_pipeline.h"

#include "core/platform_path.h"
#include "core/window.h"

#include <GLFW/glfw3.h>
#include <backends/imgui_impl_vulkan.h>
#include <core/application.h>
#include <imgui.h>
#include <memory>

void VulkanImGuiRenderer::Initialize(VulkanRenderPass& renderPassRef)
{
	auto& swapchain = Application::Get().GetRenderer().VulkanSwapchain();
	ImGui::CreateContext();
	SetDarkThemeColors();
	InitializeFontTexture();

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(swapchain.Width()), static_cast<float>(swapchain.Height()));
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	auto shaderDirectory = FileSystemUtil::GetShaderDirectory();
	auto vertPath = FileSystemUtil::PathToString(shaderDirectory / "ui.vert");
	auto fragPath = FileSystemUtil::PathToString(shaderDirectory / "ui.frag");

	m_UIVertexShader = std::make_shared<VulkanShader>(vertPath, ShaderType::Vertex);
	m_UIFragmentShader = std::make_shared<VulkanShader>(fragPath, ShaderType::Fragment);
	m_UIMaterialLayout = std::make_shared<VulkanMaterialLayout>(m_UIVertexShader, m_UIFragmentShader, "UI Material Layout");
	m_UIMaterial = std::make_unique<VulkanMaterial>(m_UIMaterialLayout);

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

	auto builder = VulkanGraphicsPipelineBuilder("G-Buffer Pipeline")
					   .SetShaders(m_UIVertexShader, m_UIFragmentShader)
					   .SetVertexInputDescription({bindingDescription(), attributeDescription()})
					   .SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
					   .SetPolygonMode(VK_POLYGON_MODE_FILL)
					   .SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
					   .SetMultisampling(VK_SAMPLE_COUNT_1_BIT)
					   .SetDepthTesting(false, false, VK_COMPARE_OP_LESS_OR_EQUAL)
					   .SetRenderPass(&renderPassRef)
					   .SetLayout(m_UIMaterialLayout->GetPipelineLayout());

	m_UIPipeline = builder.Build();
}

void VulkanImGuiRenderer::InitializeFontTexture()
{
	ImGuiIO& io = ImGui::GetIO();

	// Create font texture
	unsigned char* fontData;
	int texWidth, texHeight;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

	ImageSpecification specification
	{
		.DebugName = "Font Image",
		.Format = ImageFormat::RGBA,
		.Usage = ImageUsage::Texture,
		.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.UsedInTransferOps = true,
		.Width = static_cast<uint32_t>(texWidth),
		.Height = static_cast<uint32_t>(texHeight),
		.Mips = 1
	};
	m_FontImage = std::make_unique<VulkanImage2D>(specification);
}

void VulkanImGuiRenderer::Begin()
{
	ImGui::NewFrame();
}

// Update vertex and index buffer containing the imGui elements when required
void VulkanImGuiRenderer::UpdateBuffers()
{
	ImDrawData* imDrawData = ImGui::GetDrawData();
	// Note: Alignment is done inside buffer creation
	VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

	if (vertexBufferSize == 0 || indexBufferSize == 0)
		return;

	// Update buffers only if vertex or index count has been changed compared to current buffer size

	// Vertex buffer
	if (m_VertexBuffer == nullptr || m_VertexBuffer->GetBuffer() == VK_NULL_HANDLE || m_VertexCount != imDrawData->TotalVtxCount)
	{
		if(m_VertexBuffer)
		{
			m_VertexBuffer->Unmap();
			m_VertexBuffer.reset();
		}
		m_VertexBuffer = std::make_unique<VulkanBuffer>(
			vertexBufferSize,
			1,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		m_VertexCount = imDrawData->TotalVtxCount;
		m_VertexBuffer->Map();
	}

	// Index buffer
	if (m_IndexBuffer == nullptr || m_IndexBuffer->GetBuffer() == VK_NULL_HANDLE || m_IndexCount < imDrawData->TotalIdxCount)
	{
		if(m_IndexBuffer)
		{
			m_IndexBuffer->Unmap();
			m_IndexBuffer.reset();
		}
		m_IndexBuffer = std::make_unique<VulkanBuffer>(
			vertexBufferSize,
			1,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		m_IndexCount = imDrawData->TotalVtxCount;
		m_IndexBuffer->Map();
	}

	// Upload data
	auto* vtxDst = (ImDrawVert*)m_VertexBuffer->GetMappedMemory();
	auto* idxDst = (ImDrawIdx*)m_IndexBuffer->GetMappedMemory();

	for (int n = 0; n < imDrawData->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	m_VertexBuffer->Flush();
	m_IndexBuffer->Flush();
}

void VulkanImGuiRenderer::RecordImGuiPass(FrameInfo& frameInfo)
{
	//if(frameInfo.SwapchainSubmitCommandBuffer) Check if in recording state - if it is not, error out

	ImGui::Render();
	UpdateBuffers();

	m_UIPipeline->Bind(frameInfo.SwapchainSubmitCommandBuffer);

	// UI scale and translate via push constants
	m_TransformPushConstants.Scale = glm::vec2(2.0f / ImGui::GetIO().DisplaySize.x, 2.0f / ImGui::GetIO().DisplaySize.y);
	m_TransformPushConstants.Translate = glm::vec2(-1.0f);
	m_UIMaterial->SetPushConstant<DisplayTransformPushConstants>("p_Transform", m_TransformPushConstants);
	m_UIMaterial->UpdateDescriptorSets(frameInfo.FrameIndex,
		{{0,
			{{
				.binding = 0,
				.type = DescriptorUpdate::Type::Image,
				.imageInfo = m_FontImage->GetDescriptorInfo()}}}});

	m_UIMaterial->BindPushConstants(frameInfo.SwapchainSubmitCommandBuffer);
	m_UIMaterial->BindDescriptors(frameInfo.FrameIndex, frameInfo.SwapchainSubmitCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);

	// Render commands
	ImDrawData* imDrawData = ImGui::GetDrawData();
	int32_t vertexOffset = 0;
	int32_t indexOffset = 0;

	if (imDrawData->CmdListsCount > 0)
	{
		VkDeviceSize offsets[1] = { 0 };
		VkBuffer buffers[] = {m_VertexBuffer->GetBuffer()};
		vkCmdBindVertexBuffers(frameInfo.SwapchainSubmitCommandBuffer, 0, 1, buffers, offsets);
		vkCmdBindIndexBuffer(frameInfo.SwapchainSubmitCommandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

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
				vkCmdSetScissor(frameInfo.SwapchainSubmitCommandBuffer, 0, 1, &scissorRect);
				vkCmdDrawIndexed(frameInfo.SwapchainSubmitCommandBuffer, imDrawCmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				indexOffset += imDrawCmd->ElemCount;
			}
			vertexOffset += cmd_list->VtxBuffer.Size;
		}
	}
}

void VulkanImGuiRenderer::End()
{
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	// Update and Render additional Platform Windows
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void VulkanImGuiRenderer::OnEvent(Event& e) const
{
	if (m_BlockEvents)
	{
		ImGuiIO& io = ImGui::GetIO();
		e.Handled |= e.InCategory(EventCategoryMouse) & io.WantCaptureMouse;
		e.Handled |= e.InCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
	}
}

void VulkanImGuiRenderer::SetDarkThemeColors()
{
	auto& colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

	// Headers
	colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
	colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
	colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

	// Buttons
	colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
	colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
	colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

	// Frame BG
	colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
	colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
	colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

	// Tabs
	colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
	colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
	colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
	colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

	// Title
	colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
	colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
}

void VulkanImGuiRenderer::Shutdown()
{

}
