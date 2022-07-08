#include "lypch.h"
#include "Application.h"

#include "Core/Window.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	Metrics Application::s_Metrics;

	Application::Application(const ApplicationArgs& args, const ApplicationCreateInfo& createInfo)
		: m_Args(args), m_CreateInfo(createInfo) {
		RendererCreateInfo rendererCreateInfo;
		rendererCreateInfo.Architecture = RenderArchitecture::Vulkan;

		m_CreateInfo.WindowCreateInfo.Name = fmt::format("{0} - Windows x64 {1}", m_CreateInfo.WindowCreateInfo.Name,
																  rendererCreateInfo.Architecture == RenderArchitecture::OpenGL ? "OpenGL" : "Vulkan");

		m_Window = Window::Create(m_CreateInfo.WindowCreateInfo);
		m_Window->Init(rendererCreateInfo.Architecture);
		m_Window->SetEventCallback(LUCY_BIND_FUNC(&Application::OnEvent));

		rendererCreateInfo.Window = m_Window;

		Renderer::Init(rendererCreateInfo);

		FileSystem::Init();
	}

	Application::~Application() {
		Renderer::WaitForDevice();
		for (Ref<Module> m : m_ModuleStack.GetStack())
			m->Destroy();
		Renderer::Destroy();
		m_Window->Destroy();
		FileSystem::Destroy();
	}
}
