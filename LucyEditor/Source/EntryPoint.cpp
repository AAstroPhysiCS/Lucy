#include "lypch.h"
#include "Lucy.h"

#ifdef LUCY_WINDOWS

int main(int argc, char** argv) {
	Lucy::Logger::Init();

	Lucy::WindowCreateInfo windowCreateInfo;
	windowCreateInfo.Width = 1366;
	windowCreateInfo.Height = 766;
	windowCreateInfo.Title = "LucyEditor";
	windowCreateInfo.Resizable = true;
	windowCreateInfo.VSync = false;

	Lucy::ApplicationCreateInfo applicationCreateInfo;
	applicationCreateInfo.WindowCreateInfo = windowCreateInfo;
	//applicationCreateInfo.RenderArchitecture = RenderArchitecture::Vulkan;

	Lucy::Application* lucyApplication = Lucy::CreateEditorApplication({ argc, argv }, applicationCreateInfo);
	lucyApplication->Run();
	delete lucyApplication;
	
	return 0;
}

#endif