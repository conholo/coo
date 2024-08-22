#include "vulkan/vulkan_swapchain_renderer.h"

#include "ui/imgui_vulkan_renderer.h"

#include "core/platform_path.h"
#include "core/window.h"
#include "vulkan/render_passes/render_graph_resource_declarations.h"
#include "vulkan/render_passes/render_pass_resource.h"

#include <GLFW/glfw3.h>
#include <core/application.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>

#include <memory>

void VulkanImGuiRenderer::Initialize(RenderGraph& graph)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	SetDarkThemeColors();

	auto& swapchain = Application::Get().GetRenderer().VulkanSwapchain();

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(swapchain.Width()), static_cast<float>(swapchain.Height()));
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
	const std::string pathToFont = FileSystemUtil::PathToString(FileSystemUtil::GetFontDirectory() / "Cascadia.ttf");
	io.Fonts->AddFontFromFileTTF(pathToFont.c_str(), 13.0f);
	io.FontDefault = io.Fonts->AddFontFromFileTTF(pathToFont.c_str(), 13.0f);

	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);

	// Create font texture
	unsigned char* fontData;
	int texWidth, texHeight;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
	m_FontMemoryBuffer = std::make_unique<Buffer>(fontData, texWidth * texHeight * 4 * sizeof(char));

	auto createFontTexture =
		[this, texWidth, texHeight](const std::string& resourceName)
	{
		TextureSpecification specification
		{
			.Format = ImageFormat::RGBA,
			.Usage = TextureUsage::Texture,
			.Width = static_cast<uint32_t>(texWidth),
			.Height = static_cast<uint32_t>(texHeight),
			.GenerateMips = false,
			.UsedInTransferOps = true,
			.CreateSampler = true,
			.SamplerSpec
				{
					.Anisotropy = 1.0f
				},
			.DebugName = resourceName
		};

		auto texture = VulkanTexture2D::CreateFromMemory(specification,  *m_FontMemoryBuffer);
		return std::make_shared<TextureResource>(resourceName, texture);
	};

	m_FontTextureHandle = graph.CreateResource<TextureResource>(UIFontTextureResourceName, createFontTexture);
	auto fontTextureResource = graph.GetResource(m_FontTextureHandle);
	VulkanDescriptorPool::Builder poolBuilder;
	poolBuilder
		.AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
		.SetMaxSets(1);
	m_DescriptorPool = poolBuilder.Build();

	VulkanDescriptorSetLayout::Builder builder;
	builder.AddDescriptor(
		0,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		1);
	m_SetLayout = builder.Build();
	VulkanDescriptorWriter(*m_SetLayout, *m_DescriptorPool)
		.WriteImage(0, fontTextureResource->Get()->GetBaseViewDescriptorInfo())
		.Build(m_FontDescriptorSet);
	io.Fonts->SetTexID((ImTextureID)m_FontDescriptorSet);

	// Initialize the empty buffers - these will get created and worked on in the Update function when draw lists are registered.
	auto createNullBuffer =
		[](size_t index, const std::string& resourceBaseName, VkBufferUsageFlags flags)
	{
		auto resourceName = resourceBaseName + " " + std::to_string(index);
		auto buffer = std::make_shared<VulkanBuffer>();
		return std::make_shared<BufferResource>(resourceName, buffer);
	};

	m_VertexBufferHandles = graph.CreateResources<BufferResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		UIVertexBufferResourceName,
		createNullBuffer,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	m_IndexBufferHandles = graph.CreateResources<BufferResource>(
		VulkanSwapchain::MAX_FRAMES_IN_FLIGHT,
		UIIndexBufferResourceName,
		createNullBuffer,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	Application& app = Application::Get();
	auto* window = static_cast<GLFWwindow*>(app.GetWindow().GetWindowPtr());
	ImGui_ImplGlfw_InitForVulkan(window, true);
}

void VulkanImGuiRenderer::StartRecording()
{
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

// Update vertex and index buffer containing the imGui elements when required
void VulkanImGuiRenderer::EndRecording(RenderGraph& graph, uint32_t frameIndex)
{
	ImGui::Render();
	ImDrawData* imDrawData = ImGui::GetDrawData();
	VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

	if (vertexBufferSize == 0 || indexBufferSize == 0)
	{
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow().GetExtent().width, (float)app.GetWindow().GetExtent().height);

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
		return;
	}

	ResourceHandle<BufferResource> vboHandle = m_VertexBufferHandles[frameIndex];
	auto* vboResource = graph.GetResource<BufferResource>(vboHandle);
	auto vbo = vboResource->Get();
	if (vbo->GetBuffer() == VK_NULL_HANDLE || m_VertexCount != imDrawData->TotalVtxCount)
	{
		// First free any resources from the existing buffer.
		if(vbo->GetBuffer() != VK_NULL_HANDLE)
		{
			vbo->Destroy();
		}

		vbo->Initialize(
			vertexBufferSize,
			1,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		vbo->Map();

		//m_VertexCount = imDrawData->TotalVtxCount;
	}

	ResourceHandle<BufferResource> eboHandle = m_IndexBufferHandles[frameIndex];
	auto* eboResource = graph.GetResource<BufferResource>(eboHandle);
	auto ebo = eboResource->Get();
	if (ebo->GetBuffer() == VK_NULL_HANDLE || m_IndexCount != imDrawData->TotalIdxCount)
	{
		// First free any resources from the existing buffer.
		if(ebo->GetBuffer() != VK_NULL_HANDLE)
		{
			ebo->Destroy();
		}

		ebo->Initialize(
			indexBufferSize,
			1,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		ebo->Map();
		//m_IndexCount = imDrawData->TotalIdxCount;
	}

	// Upload data
	auto* vtxDst = (ImDrawVert*)vbo->GetMappedMemory();
	auto* idxDst = (ImDrawIdx*)ebo->GetMappedMemory();

	for (int n = 0; n < imDrawData->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	vbo->Flush();
	ebo->Flush();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	// Update and Render additional Platform Windows
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
	Application& app = Application::Get();

	io.DisplaySize = ImVec2((float)app.GetWindow().GetExtent().width, (float)app.GetWindow().GetExtent().height);

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
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

void VulkanImGuiRenderer::Shutdown(RenderGraph& graph)
{
	auto freeBuffer =
		[](std::shared_ptr<VulkanBuffer> buffer)
	{
		buffer->Unmap();
		buffer.reset();
	};

	graph.TryFreeResources<BufferResource>(UIVertexBufferResourceName, freeBuffer);
	graph.TryFreeResources<BufferResource>(UIIndexBufferResourceName, freeBuffer);
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}
