#pragma once

#include "Core/Base.h"
#include "Buffer.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	class VertexBuffer : public Buffer<float> {
	public:
		static Ref<VertexBuffer> Create(uint32_t size);
		
		virtual ~VertexBuffer() = default;

		virtual void LoadToGPU() = 0;
		virtual void DestroyHandle() = 0;
	protected:
		VertexBuffer(uint32_t size);
	};
}

