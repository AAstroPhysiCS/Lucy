#pragma once

#include "Base.h"
#include "Window.h"
#include "ApplicationMetrics.h"

#include "Overlay.h"

#include "Threading/RunnableThreadScheduler.h"
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

		inline static ApplicationMetrics& GetApplicationMetrics() { return s_Metrics; }
		inline static auto GetRunnableThreadScheduler() { return s_RunnableThreadScheduler; }
		inline static auto GetTaskScheduler() { return s_TaskScheduler; }
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

		RendererConfiguration m_RendererConfiguration;
		Ref<RenderPipeline> m_RenderPipeline = nullptr;

		ApplicationArgs m_Args;
		ApplicationCreateInfo m_CreateInfo;
		static inline ApplicationMetrics s_Metrics;

		static inline auto s_RunnableThreadScheduler = new RunnableThreadScheduler<RenderThread>({
			RunnableThreadCreateInfo {
			   .Name = "LucyRenderThread",
			   .Affinity = ThreadApplicationAffinityIncremental,
			   .Priority = ThreadPriority::Highest
			},
		});
		static inline auto s_TaskScheduler = new TaskScheduler(TaskSchedulerCreateInfo{ .FromThreadIndex = s_RunnableThreadScheduler->GetThreadExtent() });

		friend int ::main(int argc, char** argv);
	};

	extern Application* CreateApplication(const ApplicationArgs& args, const ApplicationCreateInfo& createInfo);
}