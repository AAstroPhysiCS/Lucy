#pragma once

#include "Buffer.h"
#include "Renderer/Descriptors/DescriptorType.h"

#include "Renderer/Device/RenderResource.h"

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

	class SharedStorageBuffer : public ByteBuffer, public RenderResource {
	public:
		SharedStorageBuffer(const SharedStorageBufferCreateInfo& createInfo) 
			: RenderResource("SharedStorageBuffer"), m_CreateInfo(createInfo) {
			Reserve(m_CreateInfo.BufferSize);
		}
		virtual ~SharedStorageBuffer() = default;

		virtual void RTLoadToDevice() = 0;
		
		inline const std::string& GetName() const { return m_CreateInfo.Name; }
		inline uint32_t GetBinding() const { return m_CreateInfo.Binding; }
		inline uint32_t GetSize() const { return m_CreateInfo.BufferSize; }
		inline uint32_t GetArraySize() const { return m_CreateInfo.ArraySize; }
		inline DescriptorType GetDescriptorType() const { return m_CreateInfo.Type; }
	protected:
		SharedStorageBufferCreateInfo m_CreateInfo;
	};
}