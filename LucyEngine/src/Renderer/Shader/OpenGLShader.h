#pragma once
#include <iostream>
#include <algorithm>
#include <vector>

#include "Shader.h"
#include "../../Utils.h"

namespace Lucy {
	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::string& path, const std::string& name);
		virtual ~OpenGLShader() = default;

		void Load();
		void Bind();
		void Unbind();
		void Destroy();
	private:
		std::string LoadVertexData(std::vector<std::string>& lines);
		std::string LoadFragmentData(std::vector<std::string>& lines);
	};
}

