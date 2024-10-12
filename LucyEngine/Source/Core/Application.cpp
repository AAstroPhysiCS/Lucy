#include "lypch.h"
#include "Application.h"

#include "FileSystem.h"

#include "Events/EventHandler.h"

#include "Events/InputEvent.h"
#include "Events/KeyCodes.h"
#include "Events/WindowEvent.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderThread.h"

#include "Core/ViewportRenderPipeline.h"

namespace Lucy {

	Application::Application(const ApplicationArgs& args, const ApplicationCreateInfo& createInfo)
		: m_Args(args), m_CreateInfo(createInfo) {
	}

	Application::~Application() {
		FileSystem::Destroy();

		Renderer::WaitForDevice();

		m_Scene->Destroy();
		for (const auto& overlay : m_Overlays)
			overlay->Destroy();

		Renderer::Destroy();
		m_Window->Destroy();

		delete s_TaskScheduler;
		delete s_RunnableThreadScheduler;
	}

	void Application::Init() {
		LUCY_ASSERT(m_Scene);

		Logger::Init();

		m_Window = Window::Create(m_CreateInfo.WindowCreateInfo);
		m_Window->Init(m_CreateInfo.RendererConfiguration.RenderArchitecture);
		m_Window->SetEventCallback(LUCY_BIND_FUNC(&Application::OnEvent, this, std::placeholders::_1));

		EventHandler::Init(this, m_Window->Raw());
		FileSystem::Init();

#ifdef LUCY_DEBUG
		m_Window->SetTitle(fmt::format("{0} - Windows x64 Debug {1}", m_Window->GetTitle(),
						   m_CreateInfo.RendererConfiguration.RenderArchitecture == RenderArchitecture::Vulkan ? "Vulkan" : "D3D12").c_str());
#else
		m_Window->SetTitle(fmt::format("{0} - Windows x64 Release {1}", m_Window->GetTitle(),
						   m_CreateInfo.RendererConfiguration.RenderArchitecture == RenderArchitecture::Vulkan ? "Vulkan" : "D3D12").c_str());
#endif

		Renderer::Init(m_CreateInfo.RendererConfiguration, m_Window);

		switch (m_CreateInfo.RendererConfiguration.RenderType) {
			case RenderType::Rasterizer: {
				RenderPipelineCreateInfo createInfo = {
					.ViewMode = ViewMode::Lit,
				};
				m_RenderPipeline = Memory::CreateRef<ViewportRenderPipeline>(createInfo, m_Scene);
				break;
			}
			default:
				LUCY_ASSERT(false, "Only Rasterizer is supported for now!");
		}

		if (m_CreateInfo.RendererConfiguration.ThreadingPolicy == ThreadingPolicy::Multithreaded)
			s_RunnableThreadScheduler->Start();
	}

	void Application::Run() {
		Init();

		Renderer::CompileRenderGraph();

		for (const auto& overlay : m_Overlays) {
			overlay->SetRenderPipeline(m_RenderPipeline);
			overlay->OnRendererInit(m_Window);
		}

		while (!glfwWindowShouldClose(m_Window->Raw())) {
			LUCY_PROFILE_NEW_FRAME("Lucy");

			m_Window->PollEvents();

			m_Scene->Update();

			m_RenderPipeline->BeginFrame();
			for (const auto& overlay : m_Overlays) {
				overlay->Begin();
				overlay->Render();
				overlay->End();
			}
			m_RenderPipeline->RenderFrame();
			m_RenderPipeline->EndFrame();

			RenderContextResultCodes result = Renderer::WaitAndPresent();
			if (result == RenderContextResultCodes::ERROR_OUT_OF_DATE_KHR || 
				result == RenderContextResultCodes::SUBOPTIMAL_KHR || 
				result == RenderContextResultCodes::NOT_READY) {
				EventHandler::DispatchImmediateEvent<SwapChainResizeEvent>();
			}
			Renderer::Flush();

			LUCY_PROFILE_NEW_EVENT("Metrics::Update");
			s_Metrics.Update();
		}
	}

	void Application::OnEvent(Event& e) {
		m_Window->WaitEventsIfMinimized();

		EventHandler::AddListener<KeyEvent>(e, [&](const KeyEvent& e) {
			if (e == KeyCode::Escape) {
				glfwSetWindowShouldClose(m_Window->Raw(), true);
			}
		});

		m_RenderPipeline->OnEvent(e);
		m_Scene->OnEvent(e);
		Renderer::OnEvent(e);

		for (const auto& overlay : m_Overlays)
			overlay->OnEvent(e);
	}

	void Application::SetScene(Ref<Scene> scene) {
		m_Scene = scene;
	}

	void Application::SetRenderType(RenderType renderType) {
		m_RendererConfiguration.RenderType = renderType;
	}

	void Application::PushOverlay(Ref<Overlay> overlay) {
		m_Overlays.push_back(overlay);
	}
}