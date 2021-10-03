#pragma once

#include <iostream>
#include "LayerStack.h"

namespace Lucy {

	struct ApplicationSpecification {
		std::string name; //name of the window
		uint32_t width, height; //size of the window
	};

	struct ApplicationArgs {
		int32_t Argc;
		char** Argv;
	};

	class Application
	{
	public:
	
		Application(const ApplicationArgs& args, const ApplicationSpecification& specs);
		virtual ~Application() { 
			for (Layer* layers : m_LayerStack.GetStack()) {
				layers->Destroy();
			}
		}

		virtual void OnRun() const = 0;

	protected:
		virtual void OnEvent() const = 0;
		
		ApplicationSpecification m_Specs;
		ApplicationArgs m_Args;

		LayerStack m_LayerStack;

		friend extern int main(int argc, char** argv);

	};

	extern Application* CreateEditorApplication(const ApplicationArgs& args, const ApplicationSpecification& specs);
	extern Application* CreateEditorApplication(const ApplicationArgs& args);
}






