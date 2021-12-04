#include "lypch.h"
#include "OpenGLShader.h"

#include "glad/glad.h"
#include "Utils.h"

namespace Lucy {
	
	OpenGLShader::OpenGLShader(const std::string& path, const std::string& name) 
		: Shader(path, name)
	{}

	void OpenGLShader::SetMat4(const char* name, glm::mat4& mat) {
		if (!m_UniformLocations.count(name)) {
			int32_t location = glGetUniformLocation(m_Program, name);
			if (location == -1) LUCY_ASSERT(false);
			m_UniformLocations.insert({ name, location });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniformMatrix4fv(it->second, 1, false, glm::value_ptr(mat));
		}
	}

	void OpenGLShader::SetMat3(const char* name, glm::mat3& mat) {
		if (!m_UniformLocations.count(name)) {
			int32_t location = glGetUniformLocation(m_Program, name);
			if (location == -1) LUCY_ASSERT(false);
			m_UniformLocations.insert({ name, location });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniformMatrix3fv(it->second, 1, false, glm::value_ptr(mat));
		}
	}

	void OpenGLShader::SetVec4(const char* name, glm::vec4& vec) {
		if (!m_UniformLocations.count(name)) {
			int32_t location = glGetUniformLocation(m_Program, name);
			if (location == -1) LUCY_ASSERT(false);
			m_UniformLocations.insert({ name, location });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniform4f(it->second, vec.x, vec.y, vec.z, vec.w);
		}
	}

	void OpenGLShader::SetVec3(const char* name, glm::vec3& vec) {
		if (!m_UniformLocations.count(name)) {
			int32_t location = glGetUniformLocation(m_Program, name);
			if (location == -1) LUCY_ASSERT(false);
			m_UniformLocations.insert({ name, location });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniform3f(it->second, vec.x, vec.y, vec.z);
		}
	}

	void OpenGLShader::SetVec2(const char* name, glm::vec2& vec) {
		if (!m_UniformLocations.count(name)) {
			int32_t location = glGetUniformLocation(m_Program, name);
			if (location == -1) LUCY_ASSERT(false);
			m_UniformLocations.insert({ name, location });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniform2f(it->second, vec.x, vec.y);
		}
	}

	void OpenGLShader::SetFloat(const char* name, float value) {
		if (!m_UniformLocations.count(name)) {
			int32_t location = glGetUniformLocation(m_Program, name);
			if (location == -1) LUCY_ASSERT(false);
			m_UniformLocations.insert({ name, location });
		}
		uint32_t loc = m_UniformLocations.find(name)->second;
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniform1f(it->second, value);
		}
	}

	void OpenGLShader::SetInt(const char* name, int32_t value) {
		if (!m_UniformLocations.count(name)) {
			int32_t location = glGetUniformLocation(m_Program, name);
			if (location == -1) LUCY_ASSERT(false);
			m_UniformLocations.insert({ name, location });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniform1i(it->second, value);
		}
	}

	void OpenGLShader::SetInt(const char* name, int32_t* value, uint32_t count) {
		if (!m_UniformLocations.count(name)) {
			int32_t location = glGetUniformLocation(m_Program, name);
			if (location == -1) LUCY_ASSERT(false);
			m_UniformLocations.insert({ name, location });
		}
		auto& it = m_UniformLocations.find(name);
		if (it != m_UniformLocations.end()) {
			glUniform1iv(it->second, count, value);
		}
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

	void OpenGLShader::LoadInternal(std::vector<uint32_t>& dataVertex, std::vector<uint32_t>& dataFragment) {
		m_Program = glCreateProgram();

		uint32_t vertexId = glCreateShader(GL_VERTEX_SHADER);
		uint32_t fragmentId = glCreateShader(GL_FRAGMENT_SHADER);

		glShaderBinary(1, &vertexId, GL_SHADER_BINARY_FORMAT_SPIR_V, dataVertex.data(), dataVertex.size() * sizeof(uint32_t));
		glSpecializeShader(vertexId, "main", 0, nullptr, nullptr);
		glAttachShader(m_Program, vertexId);

		glShaderBinary(1, &fragmentId, GL_SHADER_BINARY_FORMAT_SPIR_V, dataFragment.data(), dataFragment.size() * sizeof(uint32_t));
		glSpecializeShader(fragmentId, "main", 0, nullptr, nullptr);
		glAttachShader(m_Program, fragmentId);
		
		glLinkProgram(m_Program);

		int32_t isLinked;
		glGetProgramiv(m_Program, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE) {
			int32_t maxLength;
			glGetProgramiv(m_Program, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<char> infoLog(maxLength);
			glGetProgramInfoLog(m_Program, maxLength, &maxLength, infoLog.data());
			LUCY_CRITICAL(fmt::format("Shader linking failed ({0}):{1}", m_Path, infoLog.data()));

			glDeleteShader(vertexId);
			glDeleteShader(fragmentId);
			glDeleteProgram(m_Program);

			LUCY_ASSERT(false);
		}

		glDetachShader(m_Program, vertexId);
		glDetachShader(m_Program, fragmentId);

		glDeleteShader(vertexId);
		glDeleteShader(fragmentId);
	}
}


