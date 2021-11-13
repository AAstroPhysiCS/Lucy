#include "lypch.h"

#include "OpenGLShader.h"
#include "../Renderer.h"

#include "glad/glad.h"

namespace Lucy {

	OpenGLShader::OpenGLShader(const std::string& path, const std::string& name)
		: Shader(path, name) {
		Renderer::Submit([&]() {
			Load();
		});
	}

	void OpenGLShader::Load() {
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
			LUCY_ASSERT(false);
		}

		glGetShaderiv(fragmentId, GL_COMPILE_STATUS, &err);
		if (err != GL_TRUE) {
			LUCY_CRITICAL("Fragment Shader crashed!");

			GLsizei length;
			GLchar buffer[1024];
			glGetShaderInfoLog(fragmentId, sizeof(buffer), &length, buffer);

			LUCY_CRITICAL("Shader LOG: ");
			LUCY_CRITICAL(buffer);
			LUCY_ASSERT(false);
		}
#elif LUCY_RELEASE
		GLint err;
		glGetShaderiv(vertexId, GL_COMPILE_STATUS, &err);
		glGetShaderiv(fragmentId, GL_COMPILE_STATUS, &err);
#endif

		glAttachShader(m_Program, vertexId);
		glAttachShader(m_Program, fragmentId);

		glLinkProgram(m_Program);
		glValidateProgram(m_Program);

		glDeleteShader(vertexId);
		glDeleteShader(fragmentId);
	}

	void OpenGLShader::Bind() {
		glUseProgram(m_Program);
	}

	void OpenGLShader::Unbind() {
		glUseProgram(0);
	}

	void OpenGLShader::Destroy() {
		glDeleteProgram(m_Program);
	}

	void OpenGLShader::SetMat4(const char* name, glm::mat4& mat) {
		if (!m_UniformLocations.count(name)) {
			m_UniformLocations.insert({ name, glGetUniformLocation(m_Program, name) });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniformMatrix4fv(it->second, 1, false, glm::value_ptr(mat));
		}
	}

	void OpenGLShader::SetMat3(const char* name, glm::mat3& mat) {
		if (!m_UniformLocations.count(name)) {
			m_UniformLocations.insert({ name, glGetUniformLocation(m_Program, name) });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniformMatrix3fv(it->second, 1, false, glm::value_ptr(mat));
		}
	}

	void OpenGLShader::SetVec4(const char* name, glm::vec4& vec) {
		if (!m_UniformLocations.count(name)) {
			m_UniformLocations.insert({ name, glGetUniformLocation(m_Program, name) });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniform4f(it->second, vec.x, vec.y, vec.z, vec.w);
		}
	}

	void OpenGLShader::SetVec3(const char* name, glm::vec3& vec) {
		if (!m_UniformLocations.count(name)) {
			m_UniformLocations.insert({ name, glGetUniformLocation(m_Program, name) });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniform3f(it->second, vec.x, vec.y, vec.z);
		}
	}

	void OpenGLShader::SetVec2(const char* name, glm::vec2& vec) {
		if (!m_UniformLocations.count(name)) {
			m_UniformLocations.insert({ name, glGetUniformLocation(m_Program, name) });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniform2f(it->second, vec.x, vec.y);
		}
	}

	void OpenGLShader::SetFloat(const char* name, float value) {
		if (!m_UniformLocations.count(name)) {
			m_UniformLocations.insert({ name, glGetUniformLocation(m_Program, name) });
		}
		uint32_t loc = m_UniformLocations.find(name)->second;
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniform1f(it->second, value);
		}
	}

	void OpenGLShader::SetInt(const char* name, int32_t value) {
		if (!m_UniformLocations.count(name)) {
			m_UniformLocations.insert({ name, glGetUniformLocation(m_Program, name) });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniform1i(it->second, value);
		}
	}

	void OpenGLShader::SetInt(const char* name, int32_t* value, uint32_t count) {
		if (!m_UniformLocations.count(name)) {
			m_UniformLocations.insert({ name, glGetUniformLocation(m_Program, name) });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniform1iv(it->second, count, value);
		}
	}

	std::string OpenGLShader::LoadVertexData(std::vector<std::string>& lines) {
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

	std::string OpenGLShader::LoadFragmentData(std::vector<std::string>& lines) {
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