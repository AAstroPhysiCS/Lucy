#pragma once

#include "Buffer.h"
#include "Renderer/Descriptors/DescriptorType.h"

namespace Lucy {

	struct ShaderMemberVariable;

	struct SharedStorageBufferCreateInfo {
		std::string Name = "Unnamed Shared Storage Buffer";
		uint32_t Binding = 0;
		uint32_t BufferSize = 0;
		uint32_t ArraySize = 0; //default is 0, which means no array
		DescriptorType Type = DescriptorType::Undefined;
		std::vector<ShaderMemberVariable> ShaderMemberVariables;
	};

	class SharedStorageBuffer : public Buffer<uint8_t> {
	public:
		SharedStorageBuffer(const SharedStorageBufferCreateInfo& createInfo);
		virtual ~SharedStorageBuffer() = default;

		virtual void LoadToGPU() = 0;
		virtual void DestroyHandle() = 0;
		
		inline const std::string& GetName() const { return m_CreateInfo.Name; }
		inline uint32_t GetBinding() const { return m_CreateInfo.Binding; }
		inline uint32_t GetSize() const { return m_CreateInfo.BufferSize; }
		inline uint32_t GetArraySize() const { return m_CreateInfo.ArraySize; }
		inline DescriptorType GetDescriptorType() const { return m_CreateInfo.Type; }

		static Ref<SharedStorageBuffer> Create(const SharedStorageBufferCreateInfo& createInfo);
	protected:
		SharedStorageBufferCreateInfo m_CreateInfo;
	};
}