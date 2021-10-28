#pragma once

#include "Buffer.h"
#include "../Renderer.h"

namespace Lucy {

	class IndexBuffer : public Buffer<uint32_t>
	{
	public:
		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		void SetData(uint32_t* data, size_t size, uint32_t offset);
		
		static RefLucy<IndexBuffer> Create(uint32_t size);
		static RefLucy<IndexBuffer> Create();
	protected:
		IndexBuffer() = default;
		IndexBuffer(uint32_t size);
		virtual ~IndexBuffer() = default;
	};
}

