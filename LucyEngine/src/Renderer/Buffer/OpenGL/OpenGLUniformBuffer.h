#pragma once

#include "../UniformBuffer.h"

namespace Lucy {

	class OpenGLUniformBuffer : public UniformBuffer {
	public:
		OpenGLUniformBuffer(uint32_t size, uint32_t binding);
		virtual ~OpenGLUniformBuffer() = default;

		void Bind() override;
		void Unbind() override;
		void Destroy() override;
		void SetData(void* data, uint32_t size, uint32_t offset) override;
	private:
		uint32_t m_Id;
	};
}

