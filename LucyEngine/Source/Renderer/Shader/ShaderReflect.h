#pragma once

#include "spirv_cross/spirv_cross.hpp"
#include "spirv_cross/spirv_glsl.hpp"

#include "Renderer/Descriptors/DescriptorType.h"

namespace Lucy {

	struct ShaderStageInfo {
		size_t UniformCount = 0;
		size_t SampledImagesCount = 0;
		size_t StorageImageCount = 0;
		size_t PushConstantBufferCount = 0;
		size_t StageInputCount = 0;
		size_t StageOutputCount = 0;
		size_t StorageBufferCount = 0;
	};

	enum class ShaderMemberType {
		Unknown,
		Void,
		Boolean,
		SByte,
		UByte,
		Short,
		UShort,
		Int,
		UInt,
		Int64,
		UInt64,
		AtomicCounter,
		Half,
		Float,
		Double,
		Struct,
		Image,
		SampledImage,
		Sampler,
		AccelerationStructure,
		RayQuery
	};

	struct ShaderMemberVariable {
		std::string Name = "Unknown";
		uint32_t Size = 0;
		uint32_t Offset = 0;
		ShaderMemberType Type = ShaderMemberType::Unknown;
		std::vector<ShaderMemberVariable> Children;
	};

	struct ShaderUniformBlock {
		std::string Name = "Unknown Uniform Block";
		uint32_t Binding = 0;
		uint32_t BufferSize = 0;
		uint32_t ArraySize = 0; //default is 0, which means no array
		bool DynamicallyAllocated = false; //only for ssbos or ubos
		DescriptorType Type = DescriptorType::Undefined;
		VkShaderStageFlags StageFlag = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		std::vector<ShaderMemberVariable> Members;
	};

	struct VertexShaderLayoutElement {
		std::string Name = "Unknown Input";
		int32_t Location = -1;
		ShaderMemberType Type = ShaderMemberType::Unknown;
		uint32_t ShaderDataSize;
	};

	using VertexShaderLayout = std::vector<VertexShaderLayoutElement>;

	class ShaderReflect {
	public:
		~ShaderReflect() = default;

		inline ShaderStageInfo GetShaderStageInfo() const { return m_ShaderStageInfo; }

		inline std::vector<ShaderUniformBlock>& GetShaderPushConstants() { return m_ShaderPushConstants; }
		inline std::unordered_multimap<uint32_t, std::vector<ShaderUniformBlock>>& GetShaderUniformBlockMap() { return m_ShaderUniformBlockMap; }

		inline const VertexShaderLayout& GetVertexShaderLayout() const { return m_VertexShaderLayout; }

		void DestroyCachedData();
		void Info(const std::filesystem::path& path, const std::vector<uint32_t>& data, VkShaderStageFlags stageFlag);
	private:
		ShaderReflect() = default;

		void SearchFor(spirv_cross::CompilerGLSL* compiler, const spirv_cross::SmallVector<spirv_cross::Resource>& resource,
									 VkShaderStageFlags stageFlag, VkDescriptorType descriptorType);
		void ParseShaderInput(spirv_cross::CompilerGLSL* compiler, const spirv_cross::SmallVector<spirv_cross::Resource>& shaderInputs);
		void ParseStructMemberRecursive(spirv_cross::CompilerGLSL* compiler, spirv_cross::SPIRType parentType, std::vector<ShaderMemberVariable>& out);

		//Push constants get their own function, since their implementation is a bit different than other uniform buffer types
		void SearchForPushConstants(spirv_cross::CompilerGLSL* compiler, const spirv_cross::ShaderResources& resource, VkShaderStageFlags stageFlag);

		bool CheckIfAlreadyPresent(std::string_view uniformBlockName, std::vector<ShaderUniformBlock>& buffer);

		//key = individual set
		//value = uniform blocks
		std::vector<ShaderUniformBlock> m_ShaderPushConstants;
		std::unordered_multimap<uint32_t, std::vector<ShaderUniformBlock>> m_ShaderUniformBlockMap;

		VertexShaderLayout m_VertexShaderLayout;

		ShaderStageInfo m_ShaderStageInfo;

		friend class Shader;
		friend class ComputeShader;
	};
}