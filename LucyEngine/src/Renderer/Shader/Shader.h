#pragma once

#include <map>

#include "../../Core/Base.h"
#include "glm/gtc/type_ptr.hpp"

#include "shaderc/shaderc.hpp"

#include "vulkan/vulkan.h"

namespace Lucy {

	struct ShaderUniformBufferInfo {
		std::string Name;
		uint32_t Set;
		uint32_t Binding;
		uint32_t BufferSize;
		int32_t MemberCount;
		VkDescriptorType Type;
		VkShaderStageFlags StageFlag;
	};

	struct ShaderStageInfo {
		uint32_t UniformCount;
		uint32_t SampledImagesCount;
		uint32_t PushConstantBufferCount;
		uint32_t StageInputCount;
		uint32_t StageOutputCount;
	};

	class Shader {
	public:
		//args should only be used when vulkan is being targeted
		static RefLucy<Shader> Create(const std::string& path, const std::string& name);

		inline std::string& GetName() { return m_Name; }

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;
	public:
		Shader() = default;
		Shader(const std::string& path, const std::string& m_Name);

		void Load();

		inline ShaderStageInfo& GetVertexInfo() { return m_ShaderInfoVertex; }
		inline ShaderStageInfo& GetFragmentInfo() { return m_ShaderInfoFragment; }
		inline std::multimap<uint32_t, std::vector<ShaderUniformBufferInfo>>& GetDescriptorSetMap() { return m_DescriptorSetMap; }

		uint32_t m_Program = 0;
		std::string m_Path = "";
		std::string m_Name = "Unnamed";
	private:
		struct Extensions {
			const char* vertexExtension;
			const char* fragmentExtension;
		};

		ShaderStageInfo m_ShaderInfoVertex;
		ShaderStageInfo m_ShaderInfoFragment;

		std::multimap<uint32_t, std::vector<ShaderUniformBufferInfo>> m_DescriptorSetMap;

		void Info(std::vector<uint32_t>& dataVertex, std::vector<uint32_t>& dataFragment);
	protected:
		std::string LoadVertexData(std::vector<std::string>& lines);
		std::string LoadFragmentData(std::vector<std::string>& lines);
		const Extensions GetCachedFileExtension();

		void LoadAndRedoCache(shaderc::Compiler& compiler, shaderc::CompileOptions& options, const std::string& cacheFileVert, const std::string& cacheFileFrag);
		void LoadFromCache(const std::string& cacheFileVert, const std::string& cacheFileFrag);
	private:
		virtual void LoadInternal(std::vector<uint32_t>& dataVertex, std::vector<uint32_t>& dataFragment) = 0;
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
			: ElementList(elementList) {
		}

		friend class OpenGLPipeline;
		friend class VulkanPipeline;
		friend class Pipeline;
	private:
		std::vector<ShaderLayoutElement> ElementList;
	};

	class ShaderLibrary {
	public:
		ShaderLibrary() = default;
		
		RefLucy<Shader> GetShader(const std::string& name);
		void PushShader(const RefLucy<Shader>& instance);
	private:
		std::vector<RefLucy<Shader>> m_Shaders;
		friend class Renderer;
		friend class OpenGLRHI;
	};
}