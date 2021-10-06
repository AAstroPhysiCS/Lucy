#include "Lucy.h"

#ifdef LUCY_WINDOWS

int main(int argc, char** argv) {

	Lucy::Logger::Init();

	Lucy::Application* lucyApplication = Lucy::CreateEditorApplication({ argc, argv });
	lucyApplication->Run();
	delete lucyApplication;
	
	return 0;
}

#endif