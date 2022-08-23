#pragma once
#include <map>

#include "Core/Base.h"

#include "shaderc/shaderc.hpp"
#include "ShaderReflect.h"

#include "glm/gtc/type_ptr.hpp"

namespace Lucy {

	class Shader {
	public:
		//args should only be used when vulkan is being targeted
		static Ref<Shader> Create(const std::string& name, const std::string& path);

		inline std::string& GetName() { return m_Name; }

		virtual void Destroy() = 0;
	public:
		Shader() = default;
		Shader(const std::string& name, const std::string& path);
		virtual ~Shader() = default;

		void Load();

#ifdef LUCY_DEBUG
		inline ShaderStageInfo GetVertexInfo() const { return m_Reflect.GetVertexInfo(); }
		inline ShaderStageInfo GetFragmentInfo() const { return m_Reflect.GetFragmentInfo(); }
#endif

		inline std::vector<ShaderUniformBlock>& GetPushConstants() { return m_Reflect.GetShaderPushConstants(); }
		inline std::unordered_multimap<uint32_t, std::vector<ShaderUniformBlock>>& GetShaderUniformBlockMap() { return m_Reflect.GetShaderUniformBlockMap(); }

		uint32_t m_Program = 0;
		std::string m_Path = "";
		std::string m_Name = "Unnamed";
	protected:
		struct Extensions {
			const char* vertexExtension;
			const char* fragmentExtension;
		};

		std::string LoadVertexData(std::vector<std::string>& lines);
		std::string LoadFragmentData(std::vector<std::string>& lines);
		const Extensions GetCachedFileExtension();

		void LoadAndRedoCache(shaderc::Compiler& compiler, shaderc::CompileOptions& options, const std::string& cacheFileVert, const std::string& cacheFileFrag);
		void LoadFromCache(const std::string& cacheFileVert, const std::string& cacheFileFrag);
	private:
		virtual void LoadInternal(std::vector<uint32_t>& dataVertex, std::vector<uint32_t>& dataFragment) = 0;

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

	struct VertexShaderLayout {
		VertexShaderLayout() = default;
		VertexShaderLayout(const std::vector<ShaderLayoutElement>& elementList)
			: m_ElementList(elementList) {
		}

		inline const std::vector<ShaderLayoutElement>& GetElementList() const { return m_ElementList; }
	private:
		std::vector<ShaderLayoutElement> m_ElementList;
	};

	class ShaderLibrary {
	public:
		static ShaderLibrary& Get();

		ShaderLibrary() = default;

		void Init();
		void Destroy();

		Ref<Shader> GetShader(const std::string& name);
		void PushShader(const Ref<Shader>& instance);
	private:
		std::vector<Ref<Shader>> m_Shaders;
	};
}