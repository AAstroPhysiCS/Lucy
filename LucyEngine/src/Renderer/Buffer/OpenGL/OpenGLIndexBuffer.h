#pragma once

#include "../IndexBuffer.h"

namespace Lucy {

	class OpenGLIndexBuffer : public IndexBuffer {
	public:
		OpenGLIndexBuffer(uint32_t size);
		OpenGLIndexBuffer();
		virtual ~OpenGLIndexBuffer() = default;

		void Bind(const IndexBindInfo& info) override;
		void Unbind() override;
		void LoadToGPU() override;
		void DestroyHandle() override;
	private:
		uint32_t m_Id;
	};
}