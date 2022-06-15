#pragma once
#include <optional>
#include "Buffer.h"

namespace Lucy {

	class VulkanDescriptorSet;

	class UniformBuffer : public Buffer<void*> {
	public:
		virtual ~UniformBuffer() = default;

		static RefLucy<UniformBuffer> Create(uint32_t size, uint32_t binding, std::optional<VulkanDescriptorSet> descriptorSet);
		
		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;
		virtual void SetData(void* data, uint32_t size, uint32_t offset) = 0;
	protected:
		UniformBuffer() = default;
	};
}