#pragma once

#include "Buffer.h"

#include "../Renderer.h"

namespace Lucy {
	class IndexBuffer : public Buffer
	{
	public:

		virtual ~IndexBuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		void SetData(void* data);
		
		static RefLucy<IndexBuffer> Create(uint32_t size, void* data);

	protected:
		IndexBuffer(uint32_t size, void* data);
		void* m_Data;
	};
}

