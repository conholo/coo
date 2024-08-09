#include <chrono>
#include "application.h"
#include "engine_utils.h"
#include "core/event/mouse_event.h"

void Application::CreateGameObjects(Scene& scene, VulkanRenderer& renderer)
{
    TextureSpecification spec
    {
        .Usage = TextureUsage::Texture,
    };
	spec.DebugName = "Lava Texture";
	auto lavaTexture = VulkanTexture2D::CreateFromFile(spec, "../assets/textures/marble.png");
	spec.DebugName = "Marble Texture";
	auto marbleTexture = VulkanTexture2D::CreateFromFile(spec, "../assets/textures/lava.png");

    auto cubeModel = VulkanModel::CreateModelFromFile("../assets/models/cube.obj");
    auto quadModel = VulkanModel::CreateModelFromFile("../assets/models/quad.obj");

    auto& cubeA = scene.CreateGameObject(renderer);
	cubeA.ObjectModel = cubeModel;
	cubeA.ObjectTransform.Translation = {-0.5f, 0.0f, 2.0f};
	cubeA.ObjectTransform.Scale = {0.25, 0.25, 0.25};
	cubeA.ObjectTransform.Rotation = {0.0f, 0.0f, 0.0f};
	cubeA.DiffuseMap = marbleTexture;
	cubeA.NormalMap = lavaTexture;

    auto &floor = scene.CreateGameObject(renderer);
    floor.ObjectModel = quadModel;
    floor.ObjectTransform.Translation = {0.0f, 0.0f, 0.0};
    floor.ObjectTransform.Scale = {3.f, 1.0f, 3.f};
    floor.ObjectTransform.Rotation = {270.0, 0.0, 0.0};
	floor.DiffuseMap = marbleTexture;
	floor.NormalMap = lavaTexture;
}

Application::Application()
{
    m_Window = std::make_unique<Window>();
    m_Window->SetEventCallback(BIND_FN(Application::OnEvent));
    VulkanContext::Initialize("coo", 1.0, m_Window.get());

    m_Renderer = std::make_unique<VulkanRenderer>(*m_Window);
    m_Renderer->Initialize();

    m_Scene = std::make_unique<Scene>();
    CreateGameObjects(*m_Scene, *m_Renderer);

    m_Camera.SetPerspectiveProjection(90.0f, m_Window->GetExtent().width / m_Window->GetExtent().height, 0.1, 1000.0);
}

Application::~Application()
{
    m_Renderer->Shutdown();
    VulkanContext::Shutdown();
}

void Application::Run()
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    while (!m_Window->ShouldClose())
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
    m_Camera.OnEvent(event);
}

bool Application::OnWindowClose(WindowClosedEvent& event)
{
    std::cout << "Window close event\n";
    return true;
}

bool Application::OnWindowResize(WindowResizedEvent& event)
{
    std::cout << "Window resize event: " << event.GetWidth() << ", " << event.GetHeight() << "\n";
    m_Camera.SetPerspectiveProjection(90.0f, event.GetWidth() / event.GetHeight(), 0.1f, 1000.0f);
    return true;
}