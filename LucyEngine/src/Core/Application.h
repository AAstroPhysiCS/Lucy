#pragma once

#include "Base.h"

#include "LayerStack.h"
#include "Layer.h"

namespace Lucy {

	struct ApplicationArgs {
		int32_t Argc;
		char** Argv;
	};

	class Application {
	public:
		Application(const ApplicationArgs& args);
		virtual ~Application() {
			for (Layer* layer : m_LayerStack.GetStack()) {
				layer->Destroy();
			}
		}

		inline static PerformanceMetrics& GetPerformanceMetrics() { return s_Metrics; }
		
		virtual void Run() = 0;
	protected:
		ApplicationArgs m_Args;
		LayerStack m_LayerStack;
		static PerformanceMetrics s_Metrics;

		friend extern int main(int argc, char** argv);
	};

	extern Application* CreateEditorApplication(const ApplicationArgs& args);
}