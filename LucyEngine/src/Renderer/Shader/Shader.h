#pragma once

#include "../../Core/Base.h"

namespace Lucy {

	class OpenGLShader;

	class Shader
	{
	public:
		static RefLucy<Shader> Create(const std::string& path);

	protected:
		Shader(const std::string& path);

		virtual void Load() = 0;
		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;

		uint32_t m_Program = 0;
		std::string m_Path = "";
	};

	struct ShaderLayoutElement {
		const char* name = nullptr;
		size_t size;
	};

	struct VertexShaderLayout {
		VertexShaderLayout() = default;
		VertexShaderLayout(std::vector<ShaderLayoutElement>& elementList) {
			ElementList = elementList;
		}
	private:
		std::vector<ShaderLayoutElement> ElementList;
	};
}