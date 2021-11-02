#pragma once
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>

#include "Shader.h"
#include "../../Utils.h"

namespace Lucy {
	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::string& path, const std::string& name);
		virtual ~OpenGLShader() = default;
		
		void Bind();
		void Unbind();

		void SetMat4(const char* name, glm::mat4& mat);
		void SetMat3(const char* name, glm::mat3& mat);
		void SetVec4(const char* name, glm::vec4& vec);
		void SetVec3(const char* name, glm::vec3& vec);
		void SetVec2(const char* name, glm::vec2& vec);
		void SetFloat(const char* name, float value);
		void SetInt(const char* name, int32_t value);

	private:
		std::string LoadVertexData(std::vector<std::string>& lines);
		std::string LoadFragmentData(std::vector<std::string>& lines);

		void Load();
		void Destroy();

		std::map<const char*, uint32_t> m_UniformLocations;
	};
}

