#include "OpenGLShader.h"
#include "../Renderer.h"

#include "glad/glad.h"

namespace Lucy {

	OpenGLShader::OpenGLShader(const std::string& path)
		: Shader(path)
	{
	}

	void OpenGLShader::Load()
	{
		m_Program = glCreateProgram();

		std::vector<std::string>& lines = Utils::ReadFile(m_Path);
		std::string& vertex = LoadVertexData(lines);
		std::string& fragment = LoadFragmentData(lines);

		const char* vertexPtr = vertex.c_str();
		const char* fragmentPtr = fragment.c_str();

		uint32_t vertexId = glCreateShader(GL_VERTEX_SHADER);
		uint32_t fragmentId = glCreateShader(GL_FRAGMENT_SHADER);

		glShaderSource(vertexId, 1, &vertexPtr, NULL);
		glShaderSource(fragmentId, 1, &fragmentPtr, NULL);

		glCompileShader(vertexId);
		glCompileShader(fragmentId);

#ifdef LUCY_DEBUG
		GLint err;
		glGetShaderiv(vertexId, GL_COMPILE_STATUS, &err);

		if (err != GL_TRUE) {
			LUCY_CRITICAL("Vertex Shader crashed!");

			GLsizei length;
			GLchar buffer[1024];
			glGetShaderInfoLog(vertexId, sizeof(buffer), &length, buffer);

			LUCY_CRITICAL("Shader LOG: ");
			LUCY_CRITICAL(buffer);
		}

		glGetShaderiv(fragmentId, GL_COMPILE_STATUS, &err);
		if (err != GL_TRUE) {
			LUCY_CRITICAL("Fragment Shader crashed!");

			GLsizei length;
			GLchar buffer[1024];
			glGetShaderInfoLog(fragmentId, sizeof(buffer), &length, buffer);

			LUCY_CRITICAL("Shader LOG: ");
			LUCY_CRITICAL(buffer);
		}
#elif LUCY_RELEASE
		glGetShaderiv(vertexId, GL_COMPILE_STATUS, 0);
		glGetShaderiv(fragmentId, GL_COMPILE_STATUS, 0);
#endif

		glAttachShader(m_Program, vertexId);
		glAttachShader(m_Program, fragmentId);

		glLinkProgram(m_Program);
		glValidateProgram(m_Program);

		glDeleteShader(vertexId);
		glDeleteShader(fragmentId);
	}

	void OpenGLShader::Bind()
	{
		glUseProgram(m_Program);
	}

	void OpenGLShader::Unbind()
	{
		glUseProgram(0);
	}

	void OpenGLShader::Destroy()
	{
		glDeleteProgram(m_Program);
	}

	std::string OpenGLShader::LoadVertexData(std::vector<std::string>& lines)
	{
		auto& from = std::find(lines.begin(), lines.end(), "//type vertex");
		auto& to = std::find(lines.begin(), lines.end(), "//type fragment");

		std::string buffer;
		if (lines.end() != from) {
			for (auto i = from; i != to; i++) {
				buffer += *i + "\n";
			}
		}

		return buffer;
	}

	std::string OpenGLShader::LoadFragmentData(std::vector<std::string>& lines)
	{
		auto& from = std::find(lines.begin(), lines.end(), "//type fragment");
		auto& to = lines.end();

		std::string buffer;
		if (lines.end() != from) {
			for (auto i = from; i != to; i++) {
				buffer += *i + "\n";
			}
		}

		return buffer;
	}
}
