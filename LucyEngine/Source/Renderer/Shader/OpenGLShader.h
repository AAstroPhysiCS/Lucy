#pragma once

#include "Shader.h"

namespace Lucy {

	class OpenGLShader : public Shader {
	public:
		OpenGLShader(const std::string& path, const std::string& name);
		virtual ~OpenGLShader() = default;

		inline uint32_t GetProgram() { return m_Program; }
		
		void SetMat4(const char* name, glm::mat4& mat);
		void SetMat3(const char* name, glm::mat3& mat);
		void SetVec4(const char* name, glm::vec4& vec);
		void SetVec3(const char* name, glm::vec3& vec);
		void SetVec2(const char* name, glm::vec2& vec);
		void SetFloat(const char* name, float value);
		void SetInt(const char* name, int32_t value);
		void SetInt(const char* name, int32_t* value, uint32_t count);

		void Bind();
		void Unbind();
		void Destroy() override;
	private:
		void LoadInternal(std::vector<uint32_t>& dataVertex, std::vector<uint32_t>& dataFragment) override;

		std::map<const char*, uint32_t> m_UniformLocations;
	};
}

