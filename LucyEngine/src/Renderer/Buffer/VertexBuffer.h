#pragma once

#include <vector>

#include "Buffer.h"
#include "../Renderer.h"

namespace Lucy {

	class VertexBuffer : public Buffer<float>
	{
	public:
		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		void SetData(float* data, size_t size, uint32_t offset);

		static RefLucy<VertexBuffer> Create(uint32_t size);
		static RefLucy<VertexBuffer> Create();
	protected:
		VertexBuffer() = default;
		VertexBuffer(uint32_t size);
		virtual ~VertexBuffer() = default;
	};
}

