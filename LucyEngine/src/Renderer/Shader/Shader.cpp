#include "Shader.h"

#include "../Renderer.h"
#include "OpenGLShader.h"

namespace Lucy {
	
	Shader::Shader(const std::string& path, const std::string& name)
		: m_Path(path), m_Name(name)
	{
	}

	RefLucy<Shader> Shader::Create(const std::string& name, const std::string& path)
	{
		switch (Renderer::GetCurrentRenderAPI()) {
			case RenderAPI::OpenGL:
				auto ref = CreateRef<OpenGLShader>(path, name);
				Renderer::GetShaderLibrary().PushShader(ref);
				return ref;
		}
	}

	RefLucy<Shader>& ShaderLibrary::GetShader(const std::string& name)
	{
		for (auto& shader : m_Shaders)
		{
			if (shader->GetName() == name) return shader;
		}

		LUCY_CRITICAL("Shader not found!");
		LUCY_ASSERT(false);
	}
	
	void ShaderLibrary::PushShader(RefLucy<Shader> shader)
	{
		m_Shaders.push_back(shader);
	}
}
