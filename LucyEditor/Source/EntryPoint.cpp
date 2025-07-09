#include "lypch.h"
#include "Lucy.h"

#ifdef LUCY_WINDOWS

int main(int argc, char** argv) {
	
	Lucy::WindowCreateInfo windowCreateInfo;
	windowCreateInfo.Width = 1366;
	windowCreateInfo.Height = 766;
	windowCreateInfo.Title = "LucyEditor";
	windowCreateInfo.Resizable = true;
	windowCreateInfo.VSync = false;

	Lucy::ApplicationCreateInfo applicationCreateInfo;
	applicationCreateInfo.WindowCreateInfo = windowCreateInfo;
	applicationCreateInfo.RendererConfiguration = {
		.RenderArchitecture = Lucy::RenderArchitecture::Vulkan,
		.RenderType = Lucy::RenderType::Rasterizer,
		.ThreadingPolicy = Lucy::ThreadingPolicy::Multithreaded
	};

	Lucy::Application* lucyApplication = Lucy::CreateApplication({ argc, argv }, applicationCreateInfo);
	lucyApplication->Run();
	delete lucyApplication;
	
	return 0;
}

#endif