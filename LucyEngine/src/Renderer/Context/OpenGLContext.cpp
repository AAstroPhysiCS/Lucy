#include "OpenGLContext.h"
#include "../../Core/Base.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

namespace Lucy {

	OpenGLContext::OpenGLContext(RenderContextType type)
		: RenderContext(type)
	{
		Init();
	}

	void OpenGLContext::Init()
	{
		if (m_RenderContextType == RenderContextType::OpenGL && !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			Destroy();
			LUCY_CRITICAL("OpenGL init failed!");
			LUCY_ASSERT(false);
		}
	}

	void OpenGLContext::Destroy()
	{
		glfwTerminate();
	}

	void OpenGLContext::PrintInfo()
	{
		const char* vendor = (const char*)glGetString(GL_VENDOR);
		const char* renderer = (const char*)glGetString(GL_RENDERER);
		const char* version = (const char*)glGetString(GL_VERSION);

		Logger::Log(LoggerInfo::LUCY_INFO, std::string("Vendor ").append(vendor));
		Logger::Log(LoggerInfo::LUCY_INFO, std::string("Renderer ").append(renderer));
		Logger::Log(LoggerInfo::LUCY_INFO, std::string("GL Version ").append(version));
	}
}
