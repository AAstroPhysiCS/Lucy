#pragma once

#include "../IndexBuffer.h"

namespace Lucy {
	class OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(uint32_t size, void* data);
		
		void Bind();
		void Unbind();
	};
}