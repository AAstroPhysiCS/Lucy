#pragma once

#include "Buffer.h"
#include "Renderer/Descriptors/DescriptorType.h"

namespace Lucy {

	struct ShaderMemberVariable;

	struct UniformBufferCreateInfo {
		std::string Name = "Unnamed Uniform Buffer";
		uint32_t Binding = 0;
		uint32_t BufferSize = 0;
		uint32_t ArraySize = 0; //default is 0, which means no array
		DescriptorType Type = DescriptorType::Undefined;
		std::vector<ShaderMemberVariable> ShaderMemberVariables;
	};

	class UniformBuffer : public Buffer<uint8_t> {
	public:
		virtual ~UniformBuffer() = default;

		static Ref<UniformBuffer> Create(UniformBufferCreateInfo& createInfo);

		virtual void LoadToGPU() = 0;
		virtual void DestroyHandle() = 0;

		inline const std::string& GetName() const { return m_CreateInfo.Name; }
		inline uint32_t GetBinding() const { return m_CreateInfo.Binding; }
		inline uint32_t GetSize() const { return m_CreateInfo.BufferSize; }
		inline uint32_t GetArraySize() const { return m_CreateInfo.ArraySize; }
		inline DescriptorType GetDescriptorType() const { return m_CreateInfo.Type; }
	protected:
		UniformBuffer(UniformBufferCreateInfo& createInfo);

		UniformBufferCreateInfo m_CreateInfo;
	};
}