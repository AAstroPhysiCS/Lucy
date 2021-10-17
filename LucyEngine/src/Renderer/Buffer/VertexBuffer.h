#pragma once

#include <vector>

#include "Buffer.h"
#include "../Renderer.h"

namespace Lucy {

	class VertexBuffer : public Buffer
	{
	public:
		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		void SetData(void* data);

		static RefLucy<VertexBuffer> Create(uint32_t size, void* data);
	protected:
		VertexBuffer(uint32_t size, void* data);
		virtual ~VertexBuffer() = default;
	};
}

