#include "lypch.h"
#include "Application.h"

#include "Core/Window.h"
#include "Core/FileSystem.h"

namespace Lucy {

	Metrics Application::s_Metrics;

	Application::Application(const ApplicationArgs& args, const ApplicationCreateInfo& createInfo)
		: m_Args(args), m_CreateInfo(createInfo) {
		m_Window = Window::Create(m_CreateInfo.WindowCreateInfo);
		m_Window->Init(m_CreateInfo.RenderArchitecture);
		m_Window->SetEventCallback(LUCY_BIND_FUNC(&Application::OnEvent));

		FileSystem::Init();
	}

	Application::~Application() {
		for (Ref<Module> m : m_ModuleStack.GetStack())
			m->Destroy();
		m_Window->Destroy();
		FileSystem::Destroy();
	}
}
