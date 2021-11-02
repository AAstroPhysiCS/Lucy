#pragma once

#include "../../Core/Base.h"
#include "glm/gtc/type_ptr.hpp"

namespace Lucy {

	class OpenGLShader;

	class Shader
	{
	public:
		static RefLucy<Shader> Create(const std::string& path, const std::string& name);

		inline std::string& GetName() { return m_Name; }
		inline uint32_t GetProgram() { return m_Program; }

		virtual void SetMat4(const char* name, glm::mat4& mat) = 0;
		virtual void SetMat3(const char* name, glm::mat3& mat) = 0;
		virtual void SetVec4(const char* name, glm::vec4& vec) = 0;
		virtual void SetVec3(const char* name, glm::vec3& vec) = 0;
		virtual void SetVec2(const char* name, glm::vec2& vec) = 0;
		virtual void SetFloat(const char* name, float value) = 0;
		virtual void SetInt(const char* name, int32_t value) = 0;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;
	protected:
		Shader(const std::string& path, const std::string& m_Name);

		virtual void Load() = 0;

		uint32_t m_Program = 0;
		std::string m_Path = "";
		std::string m_Name = "Unnamed";
	};

	enum class ShaderDataSize : uint16_t {
		Float1, Float2, Float3, Float4,
		Int1, Int2, Int3, Int4,
		Mat4
	};

	struct ShaderLayoutElement {
		const char* name = nullptr;
		ShaderDataSize size;
	};

	struct VertexShaderLayout {
		VertexShaderLayout() = default;
		VertexShaderLayout(std::vector<ShaderLayoutElement>& elementList) {
			ElementList = elementList;
		}

		friend class OpenGLPipeline;
	private:
		std::vector<ShaderLayoutElement> ElementList;
	};

	class ShaderLibrary {
	public:
		RefLucy<Shader>& GetShader(const std::string& name);
		void PushShader(RefLucy<Shader> shader);
	private:
		ShaderLibrary() = default;

		std::vector<RefLucy<Shader>> m_Shaders;
		friend class Renderer;
	};
}