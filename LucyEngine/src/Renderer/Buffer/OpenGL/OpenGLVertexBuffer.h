#pragma once

#include "../VertexBuffer.h"

namespace Lucy {
	class OpenGLVertexBuffer : public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(uint32_t size, void* data);

		void Bind();
		void Unbind();
	};
}

