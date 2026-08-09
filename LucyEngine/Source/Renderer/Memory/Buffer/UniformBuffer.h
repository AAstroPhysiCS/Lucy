#pragma once

#include "Buffer.h"
#include "Renderer/Descriptors/DescriptorType.h"

#include "Renderer/Device/RenderDeviceResource.h"

namespace Lucy {

	struct ShaderMemberVariable;

	struct UniformBufferCreateInfo {
		std::string Name = "Unnamed Uniform Buffer";
		uint32_t Binding = 0;
		uint32_t BufferSize = 0;
		uint32_t ArraySize = 0; //default is 0, which means no array
		DescriptorType Type = UndefinedDescriptorType;
		std::vector<ShaderBlockLayoutElement> ShaderChildrenVariables;
		std::vector<ShaderMemberVariable> ShaderMemberVariables;
	};

	class UniformBuffer : public ByteBuffer, public RenderDeviceResource {
	public:
		virtual ~UniformBuffer() = default;

		UniformBuffer(const UniformBuffer&) = delete;
		UniformBuffer& operator=(const UniformBuffer&) = delete;
		UniformBuffer(UniformBuffer&&) = delete;
		UniformBuffer& operator=(UniformBuffer&&) = delete;

		virtual void RTLoadToDevice(RenderDevice* device) = 0;

		inline const std::string& GetName() const { return m_CreateInfo.Name; }
		inline uint32_t GetBinding() const { return m_CreateInfo.Binding; }
		inline uint32_t GetSize() const { return m_CreateInfo.BufferSize; }
		inline uint32_t GetArraySize() const { return m_CreateInfo.ArraySize; }
		inline DescriptorType GetDescriptorType() const { return m_CreateInfo.Type; }
	protected:
		UniformBuffer(const UniformBufferCreateInfo& createInfo)
			: RenderDeviceResource("Uniform Buffer"), m_CreateInfo(createInfo) {
			Reserve(createInfo.BufferSize);
		}

		UniformBufferCreateInfo m_CreateInfo;
	};
}