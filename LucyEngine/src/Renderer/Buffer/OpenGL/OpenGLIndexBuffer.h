#pragma once

#include "../IndexBuffer.h"

namespace Lucy {
	class OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(uint32_t size);
		OpenGLIndexBuffer() = default;
		virtual ~OpenGLIndexBuffer() = default;

		void Bind();
		void Unbind();
	};
}