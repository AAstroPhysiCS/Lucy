#include "lypch.h"
#include "Application.h"

#include "FileSystem.h"
#include "Input.h"

namespace Lucy {

	Metrics Application::s_Metrics;

	Application::Application(const ApplicationArgs& args, const ApplicationCreateInfo& createInfo)
		: m_Args(args), m_CreateInfo(createInfo) {
		m_Window = Window::Create(m_CreateInfo.WindowCreateInfo);
		m_Window->Init(m_CreateInfo.RenderArchitecture);
		m_Window->SetEventCallback(LUCY_BIND_FUNC(&Application::OnEvent));

		Input::Init(m_Window->Raw());

		FileSystem::Init();
	}

	Application::~Application() {
		for (Ref<Module> m : m_ModuleStack)
			m->Destroy();
		m_Window->Destroy();
		FileSystem::Destroy();
	}
}
