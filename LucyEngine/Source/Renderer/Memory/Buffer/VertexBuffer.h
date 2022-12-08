#pragma once

#include "Core/Base.h"
#include "Buffer.h"

#include "vulkan/vulkan.h"

namespace Lucy {

	class VertexBuffer : public Buffer<float> {
	public:
		static Ref<VertexBuffer> Create(size_t size);
		
		virtual ~VertexBuffer() = default;

		virtual void LoadToGPU() = 0;
		virtual void DestroyHandle() = 0;
	protected:
		VertexBuffer(size_t size);
	};
}

