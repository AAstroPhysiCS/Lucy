#include "Shader.h"

#include "../Renderer.h"
#include "OpenGLShader.h"

namespace Lucy {
	
	Shader::Shader(const std::string& path)
	{
		m_Path = path;
	}

	RefLucy<Shader> Shader::Create(const std::string& path)
	{
		switch (Renderer::GetCurrentRenderContextType()) {
			case RenderContextType::OpenGL:
				return CreateRef<OpenGLShader>(path);
				break;
			default:
				LUCY_CRITICAL("Other API's not supported!");
				LUCY_ASSERT(false);
				break;
		}
	}
}
