#include "Renderer.h"

namespace Lucy {

	RendererContext Renderer::m_RendererContext;
	RefLucy<RendererAPI> Renderer::m_RendererAPI;

	void Renderer::Init(RendererContext rendererContext)
	{
		m_RendererContext = rendererContext;

		if (rendererContext == RendererContext::OPENGL && !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			Destroy();
			LUCY_CRITICAL("OpenGL init failed!");
			LUCY_ASSERT(false);
		}

		PrintInfo();

		m_RendererAPI = RendererAPI::Create<OpenGLRendererAPI>();
	}

	void Renderer::Destroy()
	{
		glfwTerminate();
	}

	RendererContext Renderer::GetCurrentContext()
	{
		return m_RendererContext;
	}

	RefLucy<RendererAPI> Renderer::GetRendererAPI()
	{
		return m_RendererAPI;
	}

	void Renderer::PrintInfo()
	{
		if (m_RendererContext == RendererContext::OPENGL) {
			const char* vendor = (const char*)glGetString(GL_VENDOR);
			const char* renderer = (const char*)glGetString(GL_RENDERER);
			const char* version = (const char*)glGetString(GL_VERSION);

			Logger::Log(LoggerInfo::LUCY_INFO, std::string("Vendor ").append(vendor));
			Logger::Log(LoggerInfo::LUCY_INFO, std::string("Renderer ").append(renderer));
			Logger::Log(LoggerInfo::LUCY_INFO, std::string("GL Version ").append(version));
		}
	}

}