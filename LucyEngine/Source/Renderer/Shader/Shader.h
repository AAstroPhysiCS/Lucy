#pragma once
#include <map>

#include "shaderc/shaderc.hpp"

#include "ShaderReflect.h"

namespace Lucy {

	class Shader {
	public:
		static Ref<Shader> Create(const std::string& name, const std::string& path);
		
		Shader(const std::string& name, const std::string& path);
		virtual ~Shader() = default;

		inline const std::string& GetName() const { return m_Name; }
		
		inline std::vector<ShaderUniformBlock>& GetPushConstants() { return m_Reflect.GetShaderPushConstants(); }
		inline std::unordered_multimap<uint32_t, std::vector<ShaderUniformBlock>>& GetShaderUniformBlockMap() { return m_Reflect.GetShaderUniformBlockMap(); }

		virtual void Load() = 0;
		virtual void Destroy() = 0;
	protected:
		std::vector<uint32_t> LoadSPIRVData(const std::string& path, shaderc::Compiler& compiler, shaderc::CompileOptions& options,
						   shaderc_shader_kind kind, const std::string& cachedData);
		std::vector<uint32_t> LoadSPIRVDataFromCache(const std::string& cachedFile);

		std::string m_Path = "";
		std::string m_Name = "Unnamed";
		ShaderReflect m_Reflect;
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

	using VertexShaderLayout = std::vector<ShaderLayoutElement>;
}