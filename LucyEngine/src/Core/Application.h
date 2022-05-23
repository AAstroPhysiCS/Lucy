#pragma once

#include "Base.h"

#include "ModuleStack.h"
#include "Window.h"

namespace Lucy {

	struct ApplicationArgs {
		int32_t Argc;
		char** Argv;
	};

	struct ApplicationSpecification {
		WindowSpecification WindowSpecification;
	};

	class Application {
	public:
		Application(const ApplicationArgs& args, ApplicationSpecification& specs);
		virtual ~Application() = default;

		inline static PerformanceMetrics& GetPerformanceMetrics() { return s_Metrics; }
		inline ApplicationSpecification GetSpecification() { return m_Specification; }
		inline ApplicationArgs GetProgramArguments() { return m_Args; }

		virtual void Run() = 0;
		virtual void OnEvent(Event* e) = 0;
	protected:
		RefLucy<Window> m_Window = nullptr;
		bool m_Running = false;

		ApplicationArgs m_Args;
		ApplicationSpecification m_Specification;
		ModuleStack m_ModuleStack;
		static PerformanceMetrics s_Metrics;
	};

	extern Application* CreateEditorApplication(const ApplicationArgs& args, ApplicationSpecification& specs);
}