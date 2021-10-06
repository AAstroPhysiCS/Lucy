#pragma once

#include <iostream>
#include "LayerStack.h"
#include "Layer.h"

namespace Lucy {

	struct ApplicationArgs {
		int32_t Argc;
		char** Argv;
	};

	class Application
	{
	public:
	
		Application(const ApplicationArgs& args);

		virtual ~Application() { 
			for (Layer* layers : m_LayerStack.GetStack()) {
				layers->Destroy();
			}
		}

		virtual void Run() const = 0;

	protected:
		
		ApplicationArgs m_Args;

		LayerStack m_LayerStack;

		friend extern int main(int argc, char** argv);

	};

	extern Application* CreateEditorApplication(const ApplicationArgs& args);
}