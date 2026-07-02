#pragma once

#include "slang/slang.h"
#include "slang/slang-com-ptr.h"

#include "Renderer/Descriptors/DescriptorType.h"

namespace Lucy {

	using namespace slang;

	enum class ShaderStageType;

	struct ShaderStageInfo {
		uint32_t ConstantBufferCount = 0;        // Uniform buffers
		uint32_t StorageBufferCount = 0;         // All storage buffers (read & write)
		uint32_t SamplerCount = 0;               // Separate samplers
		uint32_t SampledImagesCount = 0;         // Read-only textures
		uint32_t StorageImageCount = 0;          // Read-write textures
		uint32_t AccelerationStructureCount = 0; // Ray tracing structures
		uint32_t PushConstantCount = 0;
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
		Double
	};

	enum class ShaderBlockType {
		Unknown,
		Struct,
		ParameterBlock,
		Array
	};

	/*
	* e.g. float, int, etc...
	*/
	struct ShaderMemberVariable {
		std::string Name = "Unknown";
		uint32_t Size = 0;
		uint32_t Offset = 0;
		ShaderMemberType Type = ShaderMemberType::Unknown;
		std::vector<ShaderMemberVariable> Children;
	};

	/*
	* e.g. struct, array, etc...
	*/
	struct ShaderBlockLayoutElement {
		std::string Name = "Unknown Block Element";
		uint32_t BufferSize = 0;
		uint32_t ArraySize = 0; //default is 0, which means no array
		uint32_t Offset = 0;
		ShaderBlockType Type = ShaderBlockType::Unknown;
		std::vector<ShaderBlockLayoutElement> Children; //for nested blocks
		std::vector<ShaderMemberVariable> Members;
	};

	/*
	* e.g. UniformBuffer<Test> test;
	*/
	struct ShaderVariable {
		std::string Name = "Unknown Shader Variable";
		uint32_t Binding = 0;
		uint32_t BufferSize = 0;
		uint32_t ArraySize = 0; //default is 0, which means no array
		bool DynamicallyAllocated = false; //only for ssbos or ubos
		DescriptorType Type = UndefinedDescriptorType;
		VkShaderStageFlags StageFlag = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		ShaderBlockLayoutElement Layout;
	};

	struct VertexShaderLayoutElement {
		std::string Name = "Unknown Input";
		int32_t Location = -1;
		ShaderMemberType Type = ShaderMemberType::Unknown;
		uint32_t ShaderDataSize;
		size_t ElementCount;
	};

	using VertexShaderLayout = std::vector<VertexShaderLayoutElement>;

	class ShaderReflect {
	public:
		ShaderReflect() = default;
		~ShaderReflect() = default;

		inline const ShaderStageInfo& GetShaderInfo() const { return m_ShaderStageInfo; }

		inline std::vector<ShaderVariable>& GetShaderPushConstants() { return m_ShaderPushConstants; }
		inline std::unordered_multimap<uint32_t, std::vector<ShaderVariable>>& GetShaderUniformBlockMap() { return m_ShaderVariableMap; }

		inline const VertexShaderLayout& GetVertexShaderLayout() const { return m_VertexShaderLayout; }

		void DestroyCachedData();
		void Info(const std::filesystem::path& path, const Slang::ComPtr<IComponentType>& program, ShaderStageType stageFlag);
	private:
		ShaderBlockLayoutElement ParseShaderVariableLayout(VariableLayoutReflection* variable);
		bool CheckIfAlreadyPresent(std::string_view blockName, std::vector<ShaderVariable>& buffer);

		//key = individual set
		//value = uniform blocks
		std::vector<ShaderVariable> m_ShaderPushConstants;
		std::unordered_multimap<uint32_t, std::vector<ShaderVariable>> m_ShaderVariableMap;

		VertexShaderLayout m_VertexShaderLayout;

		ShaderStageInfo m_ShaderStageInfo;
	};

	static uint32_t ShaderMemberTypeToSize(ShaderMemberType type) {
		switch (type) {
			case ShaderMemberType::Boolean:
				return sizeof(bool);
			case ShaderMemberType::SByte:
				return sizeof(int8_t);
			case ShaderMemberType::UByte:
				return sizeof(uint8_t);
			case ShaderMemberType::Short:
				return sizeof(int16_t);
			case ShaderMemberType::UShort:
				return sizeof(uint16_t);
			case ShaderMemberType::Int:
				return sizeof(int32_t);
			case ShaderMemberType::UInt:
				return sizeof(uint32_t);
			case ShaderMemberType::Int64:
				return sizeof(int64_t);
			case ShaderMemberType::UInt64:
				return sizeof(uint64_t);
			case ShaderMemberType::AtomicCounter:
				return sizeof(uint32_t); //assuming atomic counter is a uint32
			case ShaderMemberType::Half:
				return sizeof(float) / 2; //half is typically 16 bits
			case ShaderMemberType::Float:
				return sizeof(float);
			case ShaderMemberType::Double:
				return sizeof(double);
			case ShaderMemberType::Void:
			default:
				return 1; //unknown type
		}
	}

	static ShaderMemberType SlangScalarTypeToShaderMemberType(TypeReflection::ScalarType scalar) {
		switch (scalar) {
			case TypeReflection::ScalarType::Void:
				return ShaderMemberType::Void;
			case TypeReflection::ScalarType::Bool:
				return ShaderMemberType::Boolean;
			case TypeReflection::ScalarType::Int8:
				return ShaderMemberType::SByte;
			case TypeReflection::ScalarType::UInt8:
				return ShaderMemberType::UByte;
			case TypeReflection::ScalarType::Int16:
				return ShaderMemberType::Short;
			case TypeReflection::ScalarType::UInt16:
				return ShaderMemberType::UShort;
			case TypeReflection::ScalarType::Int32:
				return ShaderMemberType::Int;
			case TypeReflection::ScalarType::UInt32:
				return ShaderMemberType::UInt;
			case TypeReflection::ScalarType::Int64:
				return ShaderMemberType::Int64;
			case TypeReflection::ScalarType::UInt64:
				return ShaderMemberType::UInt64;
			case TypeReflection::ScalarType::Float16:
				return ShaderMemberType::Half;
			case TypeReflection::ScalarType::Float32:
				return ShaderMemberType::Float;
			case TypeReflection::ScalarType::Float64:
				return ShaderMemberType::Double;
			default:
				return ShaderMemberType::Unknown;
		}
	}

	static ShaderBlockType SlangKindToShaderBlockType(TypeReflection::Kind kind) {
		switch (kind) {
			case TypeReflection::Kind::Struct:
				return ShaderBlockType::Struct;
			case TypeReflection::Kind::Array:
				return ShaderBlockType::Array;
			case TypeReflection::Kind::ParameterBlock:
				return ShaderBlockType::ParameterBlock;
			default:
				return ShaderBlockType::Unknown;
		}
	}
}