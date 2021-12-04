#pragma once

#include "Buffer.h"

namespace Lucy {

	class UniformBuffer : public Buffer<void*> {
	public:
		static RefLucy<UniformBuffer> Create(uint32_t size, uint32_t binding);
		
		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void SetData(void* data, uint32_t size, uint32_t offset) = 0;
	protected:
		UniformBuffer() = default;
		~UniformBuffer() = default;
	};
}