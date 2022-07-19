#pragma once

#include "Base.h"
#include "ModuleStack.h"
#include "Metrics.h"
#include "Window.h"

namespace Lucy {

	struct ApplicationArgs {
		int32_t Argc;
		char** Argv;
	};

	struct ApplicationCreateInfo {
		WindowCreateInfo WindowCreateInfo;
	};

	class Application {
	public:
		Application(const ApplicationArgs& args, const ApplicationCreateInfo& createInfo);
		virtual ~Application();

		inline static Metrics& GetApplicationMetrics() { return s_Metrics; }
		inline ApplicationArgs GetProgramArguments() { return m_Args; }

		virtual void Run() = 0;
		virtual void OnEvent(Event* e) = 0;
	protected:
		Ref<Window> m_Window = nullptr;

		ApplicationArgs m_Args;
		ApplicationCreateInfo m_CreateInfo;
		ModuleStack m_ModuleStack;
		static Metrics s_Metrics;
	};

	extern Application* CreateEditorApplication(const ApplicationArgs& args, const ApplicationCreateInfo& createInfo);
}