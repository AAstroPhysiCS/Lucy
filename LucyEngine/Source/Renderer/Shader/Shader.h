#pragma once
#include <span>
#include "ShaderReflect.h"

namespace Lucy {

	enum class ShaderStageType {
		Vertex = 1 << 0,
		Fragment = 1 << 1,
		Compute = 1 << 2,
		Geometry = 1 << 3,
		RayGen = 1 << 4,
		AnyHit = 1 << 5,
		Closest = 1 << 6,
		Miss = 1 << 7,

		// Convenience for Vertex, Fragment and Compute stages
		VertexAndFragment = Vertex | Fragment,
		VertexAndFragmentAndCompute = Vertex | Fragment | Compute,

		Unknown = 1 << 8
	};

	inline ShaderStageType operator|(ShaderStageType lhs, ShaderStageType rhs) {
		return static_cast<ShaderStageType>(
			static_cast<std::underlying_type_t<ShaderStageType>>(lhs) |
			static_cast<std::underlying_type_t<ShaderStageType>>(rhs));
	}

	const auto ShaderStageToShaderString = [](auto&& shaderStage) -> const char* {
		switch (shaderStage) {
			using enum ShaderStageType;
			case Vertex: return "Vertex";
			case Fragment: return "Fragment";
			case VertexAndFragment: return "Vertex and Fragment";
			case VertexAndFragmentAndCompute: return "Vertex, Fragment and Compute";
			case Compute: return "Compute";
			case Geometry: return "Geometry";
			case RayGen: return "RayGen";
			case AnyHit: return "AnyHit";
			case Closest: return "ClosestHit";
			case Miss: return "Miss";
			default: return "Unknown";
		}
	};
	
	const auto SlangStageToShaderStage = [](auto&& slangStage) {
		switch (slangStage) {
			case SlangStage::SLANG_STAGE_VERTEX: return ShaderStageType::Vertex;
			case SlangStage::SLANG_STAGE_FRAGMENT: return ShaderStageType::Fragment;
			case SlangStage::SLANG_STAGE_COMPUTE: return ShaderStageType::Compute;
			case SlangStage::SLANG_STAGE_GEOMETRY: return ShaderStageType::Geometry;
			case SlangStage::SLANG_STAGE_RAY_GENERATION: return ShaderStageType::RayGen;
			case SlangStage::SLANG_STAGE_ANY_HIT: return ShaderStageType::AnyHit;
			case SlangStage::SLANG_STAGE_CLOSEST_HIT: return ShaderStageType::Closest;
			case SlangStage::SLANG_STAGE_MISS: return ShaderStageType::Miss;
			case SlangStage::SLANG_STAGE_NONE:
				LUCY_ASSERT(false, "Slang stage type is NONE, this should not happen");
				return ShaderStageType::Vertex;
			default: {
				LUCY_ASSERT(false, "Unknown Slang shader stage type");
				return ShaderStageType::Vertex;
			}
		}
	};

	const auto ShaderStageToVkShaderStage = [](auto&& shaderStage) -> VkShaderStageFlagBits {
		switch (shaderStage) {
			using enum ShaderStageType;
			case Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
			case Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
			case Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
			case Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
			case RayGen: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			case AnyHit: return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
			case Closest: return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			case Miss: return VK_SHADER_STAGE_MISS_BIT_KHR;
			default: {
				LUCY_ASSERT(false, "Unknown shader stage type");
				return (VkShaderStageFlagBits)0;
			}
		}
	};

	class Shader : public MemoryTrackable {
	public:
		inline std::filesystem::path GetPath() const { return m_Path; }
		inline const std::string& GetName() const { return m_Name; }

		inline std::vector<ShaderVariable>& GetShaderPushConstants() { return m_Reflect.GetShaderPushConstants(); }
		inline std::unordered_multimap<uint32_t, std::vector<ShaderVariable>>& GetShaderUniformBlockMap() { return m_Reflect.GetShaderUniformBlockMap(); }

		inline const VertexShaderLayout& GetVertexShaderLayout() const { return m_Reflect.GetVertexShaderLayout(); }

		virtual void RTLoad(const Ref<RenderDevice>& device, const std::vector<std::span<const uint32_t>>& datas) = 0;
		virtual void RTDestroyResource(const Ref<RenderDevice>& device);
	protected:
		Shader(const std::string& name, const std::filesystem::path& path);
		virtual ~Shader() = default;

		inline const ShaderStageInfo& GetShaderInfo() const { return m_Reflect.GetShaderInfo(); }
	private:
		void RunReflect(const Slang::ComPtr<slang::IComponentType>& program, ShaderStageType stageFlag);
		void PrintReflectInfo();

		std::filesystem::path m_Path = "";
		std::string m_Name = "Unnamed";
		ShaderReflect m_Reflect;

		friend class ShaderManager;
	};
}