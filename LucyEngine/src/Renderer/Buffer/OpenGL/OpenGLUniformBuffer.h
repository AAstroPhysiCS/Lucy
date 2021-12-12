#pragma once

#include "../UniformBuffer.h"

namespace Lucy {

	class OpenGLUniformBuffer : public UniformBuffer {
	public:
		OpenGLUniformBuffer(uint32_t size, uint32_t binding);
		virtual ~OpenGLUniformBuffer() = default;

		void Bind();
		void Unbind();
		void Destroy();
		void SetData(void* data, uint32_t size, uint32_t offset);
	};
}

