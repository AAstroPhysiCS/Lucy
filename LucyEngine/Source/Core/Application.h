#pragma once

#include "Window.h"
#include "ModuleStack.h"
#include "Metrics.h"

#include "InputHandler.h"
#include "FileSystem.h"
#include "JobSystem.h"

#include "Renderer/RenderArchitecture.h"

#ifdef LUCY_WINDOWS
extern int main(int argc, char** argv);
#endif

namespace Lucy {

	struct Metrics;

	struct ApplicationArgs {
		int32_t Argc;
		char** Argv;
	};

	struct ApplicationCreateInfo {
		WindowCreateInfo WindowCreateInfo;
		RenderArchitecture RenderArchitecture;
	};

	class ApplicationThreadScheduler {
		ApplicationThreadScheduler() = default;
		~ApplicationThreadScheduler() = default;

		friend class Application;
	public:
	};

	class Application {
	public:
		static Application*& Get();

		Application(const ApplicationArgs& args, const ApplicationCreateInfo& createInfo);
		virtual ~Application();

		inline static Metrics& GetApplicationMetrics() { return s_Metrics; }
		inline ApplicationArgs GetProgramArguments() const { return m_Args; }

		inline InputHandler& GetInputHandler() { return m_InputHandler; }
		inline FileSystem& GetFileSystem() { return m_FileSystem; }
		inline JobSystem& GetJobSystem() { return m_JobSystem; }
	protected:
		virtual void Run() = 0;
		virtual void OnEvent(Event* e) = 0;

		Ref<Window> m_Window = nullptr;

		ApplicationArgs m_Args;
		ApplicationCreateInfo m_CreateInfo;
		ModuleStack m_ModuleStack;
		static Metrics s_Metrics;

		InputHandler m_InputHandler;
		FileSystem m_FileSystem;
		JobSystem m_JobSystem;

		ApplicationThreadScheduler m_ThreadScheduler;

		friend int ::main(int argc, char** argv);
	};

	extern Application* CreateApplication(const ApplicationArgs& args, const ApplicationCreateInfo& createInfo);
}