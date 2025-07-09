#pragma once

#include "Base.h"
#include "Window.h"
#include "ApplicationMetrics.h"

#include "Overlay.h"

#include "Threading/TaskScheduler.h"

#include "Scene/Scene.h"

#include "Renderer/RendererConfiguration.h"
#include "Renderer/RenderThread.h"

#include "Core/RenderPipeline.h"

#ifdef LUCY_WINDOWS
extern int main(int argc, char** argv);
#endif

namespace Lucy {

	struct ApplicationArgs {
		int32_t Argc;
		char** Argv;
	};

	struct ApplicationCreateInfo {
		WindowCreateInfo WindowCreateInfo;
		RendererConfiguration RendererConfiguration;
	};

	class Application {
	public:
		Application(const ApplicationArgs& args, const ApplicationCreateInfo& createInfo);
		virtual ~Application();

		void Run();
		virtual void OnEvent(Event& e);

		inline ApplicationArgs GetProgramArguments() const { return m_Args; }

		static inline ApplicationMetrics& GetApplicationMetrics() { return s_Metrics; }
		static inline auto GetTaskScheduler() { return s_TaskScheduler; }
		
		static inline std::condition_variable& IsMainThreadReadyCondVar() { return s_MainThreadReadyCondVar; }
		static inline const std::mutex& IsMainThreadReadyMutex() { return s_MainThreadReadyMutex; }
		static inline const std::atomic_bool& IsMainThreadReady() { return s_MainThreadReady; }

		static void SetMainThreadReady(bool val);
	protected:
		void SetScene(Ref<Scene> scene);
		void SetRenderType(RenderType renderType);
		void PushOverlay(Ref<Overlay> overlay);

		inline const Ref<Window>& GetWindow() const { return m_Window; }
	private:
		void Init();

		Ref<Scene> m_Scene = nullptr;
		Ref<Window> m_Window = nullptr;
		std::vector<Ref<Overlay>> m_Overlays;

		Ref<RenderPipeline> m_RenderPipeline = nullptr;

		ApplicationArgs m_Args;
		ApplicationCreateInfo m_CreateInfo;
		static inline ApplicationMetrics s_Metrics;

		static inline auto s_TaskScheduler = new TaskScheduler(TaskSchedulerCreateInfo{ .FromThreadIndex = 1 });

		static inline std::condition_variable s_MainThreadReadyCondVar;
		static inline std::mutex s_MainThreadReadyMutex;
		static inline std::atomic_bool s_MainThreadReady = false;

		friend int ::main(int argc, char** argv);
	};

	extern Application* CreateApplication(const ApplicationArgs& args, const ApplicationCreateInfo& createInfo);
}