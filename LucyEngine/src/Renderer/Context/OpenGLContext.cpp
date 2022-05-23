#include "lypch.h"

#include "OpenGLContext.h"
#include "../../Core/Base.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

namespace Lucy {

	OpenGLContext::OpenGLContext()
		: RenderContext() {
		Init();
	}

	void OpenGLContext::Init() {
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			Destroy();
			LUCY_CRITICAL("OpenGL init failed!");
			LUCY_ASSERT(false);
		}

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glEnable(GL_MULTISAMPLE);

		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
			if (type == GL_DEBUG_TYPE_ERROR) LUCY_CRITICAL(fmt::format("OpenGL Error: message = {0}", message));
		}, 0);
	}

	void OpenGLContext::Destroy() {
		glfwTerminate();
	}

	void OpenGLContext::PrintInfo() {
		const char* vendor = (const char*)glGetString(GL_VENDOR);
		const char* renderer = (const char*)glGetString(GL_RENDERER);
		const char* version = (const char*)glGetString(GL_VERSION);

		Logger::Log(LoggerInfo::LUCY_INFO, std::string("Vendor ").append(vendor));
		Logger::Log(LoggerInfo::LUCY_INFO, std::string("Renderer ").append(renderer));
		Logger::Log(LoggerInfo::LUCY_INFO, std::string("GL Version ").append(version));
	}
}