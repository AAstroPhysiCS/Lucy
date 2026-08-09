#pragma once

#include "Buffer.h"
#include "Renderer/Descriptors/DescriptorType.h"

#include "Renderer/Device/RenderDeviceResource.h"

namespace Lucy {

	struct ShaderMemberVariable;

	struct SharedStorageBufferCreateInfo {
		std::string Name = "Unnamed Shared Storage Buffer";
		uint32_t Binding = 0;
		uint32_t BufferSize = 0;
		uint32_t ArraySize = 0; //default is 0, which means no array
		DescriptorType Type = UndefinedDescriptorType;
		std::vector<ShaderBlockLayoutElement> ShaderChildrenVariables;
		std::vector<ShaderMemberVariable> ShaderMemberVariables;
	};

	class SharedStorageBuffer : public ByteBuffer, public RenderDeviceResource {
	public:
		SharedStorageBuffer(const SharedStorageBufferCreateInfo& createInfo) 
			: RenderDeviceResource("SharedStorageBuffer"), m_CreateInfo(createInfo) {
			Reserve(m_CreateInfo.BufferSize);
		}
		virtual ~SharedStorageBuffer() = default;

		SharedStorageBuffer(const SharedStorageBuffer&) = delete;
		SharedStorageBuffer& operator=(const SharedStorageBuffer&) = delete;
		SharedStorageBuffer(SharedStorageBuffer&&) = delete;
		SharedStorageBuffer& operator=(SharedStorageBuffer&&) = delete;

		virtual void RTLoadToDevice(RenderDevice* device) = 0;
		
		inline const std::string& GetName() const { return m_CreateInfo.Name; }
		inline uint32_t GetBinding() const { return m_CreateInfo.Binding; }
		inline uint32_t GetSize() const { return m_CreateInfo.BufferSize; }
		inline uint32_t GetArraySize() const { return m_CreateInfo.ArraySize; }
		inline DescriptorType GetDescriptorType() const { return m_CreateInfo.Type; }
	protected:
		SharedStorageBufferCreateInfo m_CreateInfo;
	};
}