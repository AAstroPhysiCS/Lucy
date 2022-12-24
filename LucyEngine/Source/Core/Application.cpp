#include "lypch.h"
#include "Application.h"

namespace Lucy {

	Metrics Application::s_Metrics;

	Application*& Application::Get() {
		static Application* s_Instance = nullptr;
		return s_Instance;
	}

	Application::Application(const ApplicationArgs& args, const ApplicationCreateInfo& createInfo)
		: m_Args(args), m_CreateInfo(createInfo) {
		Logger::Init();

		m_Window = Window::Create(m_CreateInfo.WindowCreateInfo);
		m_Window->Init(m_CreateInfo.RenderArchitecture);
		m_Window->SetEventCallback(LUCY_BIND_FUNC(&Application::OnEvent));

		m_InputHandler.Init(m_Window->Raw());
		m_Filesystem.Init();
	}

	Application::~Application() {
		for (Ref<Module> m : m_ModuleStack)
			m->Destroy();
		m_Window->Destroy();
		m_Filesystem.Destroy();
	}
}