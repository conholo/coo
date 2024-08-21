#include "application.h"

#include "core/event/mouse_event.h"
#include "engine_utils.h"
#include "platform_path.h"

#include <chrono>

void Application::CreateGameObjects(Scene& scene, VulkanRenderer& renderer)
{
    TextureSpecification spec
    {
        .Usage = TextureUsage::Texture,
    };

	auto textureDirectory = FileSystemUtil::GetTextureDirectory();

	spec.DebugName = "Marble Color Texture";
	auto marbleColor = VulkanTexture2D::CreateFromFile(spec, FileSystemUtil::PathToString(textureDirectory / "marble/color.jpg"));
	spec.DebugName = "Marble Normal Texture";
	auto marbleNormal = VulkanTexture2D::CreateFromFile(spec, FileSystemUtil::PathToString(textureDirectory / "marble/normal.jpg"));

	spec.DebugName = "Paving Stones Color Texture";
	auto pavingStonesColor = VulkanTexture2D::CreateFromFile(spec, FileSystemUtil::PathToString(textureDirectory / "paving_stones/color.jpg"));
	spec.DebugName = "Paving Stones Normal Texture";
	auto pavingStonesNormal = VulkanTexture2D::CreateFromFile(spec, FileSystemUtil::PathToString(textureDirectory / "paving_stones/normal.jpg"));

	auto modelDirectory = FileSystemUtil::GetModelDirectory();
    auto cubeModel = VulkanModel::CreateModelFromFile(FileSystemUtil::PathToString(modelDirectory /  "cube.obj"));
    auto quadModel = VulkanModel::CreateModelFromFile(FileSystemUtil::PathToString(modelDirectory /  "quad.obj"));

    auto& cubeA = scene.CreateGameObject(renderer);
	cubeA.ObjectModel = cubeModel;
	cubeA.ObjectTransform.Translation = {-0.5f, 0.0f, 2.0f};
	cubeA.ObjectTransform.Scale = {0.25, 0.25, 0.25};
	cubeA.ObjectTransform.Rotation = {0.0f, 0.0f, 0.0f};
	cubeA.DiffuseMap = pavingStonesColor;
	cubeA.NormalMap = pavingStonesNormal;

    auto &floor = scene.CreateGameObject(renderer);
    floor.ObjectModel = quadModel;
    floor.ObjectTransform.Translation = {0.0f, 0.0f, 0.0};
    floor.ObjectTransform.Scale = {3.f, 1.0f, 3.f};
    floor.ObjectTransform.Rotation = {270.0, 0.0, 0.0};
	floor.DiffuseMap = marbleColor;
	floor.NormalMap = marbleNormal;
}

Application& Application::Get()
{
	assert(s_Application && "No instance of Application has been initialized.");
	return *s_Application;
}

Application::Application()
{
	assert(!s_Application && "Only one instance of Application is currently supported.");
	s_Application = this;

    m_Window = std::make_unique<Window>();
    m_Window->SetEventCallback(BIND_FN(Application::OnEvent));
    VulkanContext::Initialize("coo", 1.0, m_Window.get());

    m_Renderer = std::make_unique<VulkanRenderer>(*m_Window);
    m_Renderer->Initialize();

    m_Scene = std::make_unique<Scene>();
    CreateGameObjects(*m_Scene, *m_Renderer);

    m_Camera.SetPerspectiveProjection(90.0f, m_Window->GetExtent().width / m_Window->GetExtent().height, 0.01, 1000.0);
}

Application::~Application()
{
    m_Renderer->Shutdown();
	m_Scene = nullptr;
    VulkanContext::Shutdown();
}

void Application::Run()
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    while (m_ApplicationIsRunning)
    {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration<float>(newTime - currentTime).count();
        currentTime = newTime;

		uint32_t frameIndex = m_Renderer->GetCurrentFrameIndex();
        m_Scene->UpdateGameObjectUboBuffers(frameIndex);
        m_Camera.Tick(deltaTime);

        FrameInfo frameInfo
        {
            .FrameIndex = frameIndex,
            .DeltaTime = deltaTime,
            .ActiveScene = *m_Scene,
            .Cam = m_Camera
        };
		m_Renderer->Render(frameInfo);
    }

	vkDeviceWaitIdle(VulkanContext::Get().Device());
}

void Application::OnEvent(Event& event)
{
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<WindowClosedEvent>(BIND_FN(Application::OnWindowClose));
    dispatcher.Dispatch<WindowResizedEvent>(BIND_FN(Application::OnWindowResize));

	m_Renderer->OnEvent(event);
    m_Camera.OnEvent(event);
}

bool Application::OnWindowClose(WindowClosedEvent& event)
{
	m_ApplicationIsRunning = false;
    return true;
}

bool Application::OnWindowResize(WindowResizedEvent& event)
{
    m_Camera.SetPerspectiveProjection(90.0f, event.GetWidth() / event.GetHeight(), 0.01f, 1000.0f);
    return true;
}
