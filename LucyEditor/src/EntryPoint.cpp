#include "lypch.h"
#include "Lucy.h"

#ifdef LUCY_WINDOWS

int main(int argc, char** argv) {
	Lucy::Logger::Init();

	Lucy::WindowSpecification windowSpecs;
	windowSpecs.Width = 1366;
	windowSpecs.Height = 766;
	windowSpecs.Name = "LucyEditor";
	windowSpecs.Resizable = true;
	windowSpecs.VSync = false;

	Lucy::ApplicationSpecification applicationSpecs;
	applicationSpecs.WindowSpecification = windowSpecs;

	Lucy::Application* lucyApplication = Lucy::CreateEditorApplication({ argc, argv }, applicationSpecs);
	lucyApplication->Run();
	delete lucyApplication;
	
	return 0;
}

#endif