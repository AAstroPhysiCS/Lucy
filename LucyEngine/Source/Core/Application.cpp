#include "lypch.h"
#include "Application.h"

#include "FileSystem.h"

#include "Events/EventHandler.h"

#include "Events/InputEvent.h"
#include "Events/KeyCodes.h"
#include "Events/WindowEvent.h"

#include "Renderer/Renderer.h"
#include "Renderer/RenderThread.h"

#include "Core/GPUDrivenRenderPipeline.h"

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
	}

	void Application::Init() {
		LUCY_ASSERT(m_Scene);

		Logger::Init();

		m_Window = Window::Create(m_CreateInfo.WindowCreateInfo);
		m_Window->Init(m_CreateInfo.RendererConfiguration.RenderArchitecture);

		m_Window->SetEventCallback([](std::unique_ptr<Event> event) {
			EventHandler::s_EventQueue->Push(std::move(event));
		});

		Input::Init(m_Window->Raw());
		FileSystem::Init();

#ifdef LUCY_DEBUG
		m_Window->SetTitle(std::format("{0} - Windows x64 Debug {1}", m_Window->GetTitle(),
						   m_CreateInfo.RendererConfiguration.RenderArchitecture == RenderArchitecture::Vulkan ? "Vulkan" : "D3D12").c_str());
#else
		m_Window->SetTitle(std::format("{0} - Windows x64 Release {1}", m_Window->GetTitle(),
						   m_CreateInfo.RendererConfiguration.RenderArchitecture == RenderArchitecture::Vulkan ? "Vulkan" : "D3D12").c_str());
#endif

		Renderer::Init(m_CreateInfo.RendererConfiguration, m_Window);

		switch (m_CreateInfo.RendererConfiguration.RenderType) {
			case RenderType::Rasterizer: {
				RenderPipelineCreateInfo createInfo = {
					.ViewMode = ViewMode::Lit,
					.RenderDevice = Renderer::GetRenderDevice()
				};
				m_RenderPipeline = Memory::CreateRef<GPUDrivenRenderPipeline>(createInfo, m_Scene);
				break;
			}
			default:
				LUCY_ASSERT(false, "Only Rasterizer is supported for now!");
		}
	}

	void Application::Run() {
		Init();

		Renderer::CompileRenderGraph();

		for (const auto& overlay : m_Overlays) {
			overlay->SetRenderPipeline(m_RenderPipeline);
			overlay->OnRendererInit(m_Window);
		}
		
		Renderer::InitializeImGui();

		while (!glfwWindowShouldClose(m_Window->Raw())) {
			LUCY_PROFILE_NEW_FRAME("Lucy");

			m_Window->PollEvents();

			EventHandler::s_EventQueue->Drain([this](Event& event) {
				OnEvent(event);
			});
			
			m_Window->WaitEventsIfMinimized();

			m_Scene->Update();

			m_RenderPipeline->BeginFrame(Renderer::GetRenderDevice(), m_Scene);
			m_RenderPipeline->RenderFrame();
			m_RenderPipeline->EndFrame();

			for (const auto& overlay : m_Overlays) {
				overlay->Begin();
				overlay->Render();
				overlay->End();
			}

			if (m_CreateInfo.RendererConfiguration.ThreadingPolicy == ThreadingPolicy::Singlethreaded) {
				RenderContextResultCodes result = Renderer::WaitAndPresent();
				if (result == RenderContextResultCodes::ERROR_OUT_OF_DATE_KHR || 
					result == RenderContextResultCodes::SUBOPTIMAL_KHR || 
					result == RenderContextResultCodes::NOT_READY) {
					EventHandler::Submit<SwapChainResizeEvent>();
				}
			} else {
				std::unique_lock<std::mutex> lock(s_MainThreadReadyMutex);
				s_MainThreadReady = true;
				s_MainThreadReadyCondVar.wait(lock, [&]() { return !s_MainThreadReady.load(); });
			}

			Renderer::Flush();

			LUCY_PROFILE_NEW_EVENT("Metrics::Update");
			s_Metrics.Update();
		}
	}

	void Application::OnEvent(Event& e) {
		EventHandler::AddListener<KeyEvent>(e, [&](const KeyEvent& e) {
			if (e == KeyCode::Escape) {
				glfwSetWindowShouldClose(m_Window->Raw(), true);
				return true;
			}
			return false;
		});

		m_RenderPipeline->OnEvent(e);
		m_Scene->OnEvent(e);
		Renderer::OnEvent(e);

		for (auto it = m_Overlays.rbegin(); it != m_Overlays.rend(); ++it) {
			(*it)->OnEvent(e);
		}
	}

	void Application::SetMainThreadReady(bool val) {
		s_MainThreadReady = val;
	}

	void Application::SetScene(Ref<Scene> scene) {
		m_Scene = scene;
	}

	void Application::SetRenderType(RenderType renderType) {
		m_CreateInfo.RendererConfiguration.RenderType = renderType;
	}

	void Application::PushOverlay(Ref<Overlay> overlay) {
		m_Overlays.push_back(overlay);
	}
}