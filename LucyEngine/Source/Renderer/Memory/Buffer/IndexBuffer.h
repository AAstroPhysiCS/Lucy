#pragma once

#include "Core/Base.h"
#include "Buffer.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	class IndexBuffer : public Buffer<uint32_t> {
	public:
		static Ref<IndexBuffer> Create(uint32_t size);

		virtual ~IndexBuffer() = default;

		virtual void LoadToGPU() = 0;
		virtual void DestroyHandle() = 0;
	protected:
		IndexBuffer(uint32_t size);
	};
}

