#pragma once

#include "../VertexBuffer.h"

namespace Lucy {

	class OpenGLVertexBuffer : public VertexBuffer {
	public:
		OpenGLVertexBuffer(uint32_t size);
		OpenGLVertexBuffer();
		virtual ~OpenGLVertexBuffer() = default;

		void Bind(const VertexBindInfo& info) override;
		void Unbind() override;
		void LoadToGPU() override;
		void DestroyHandle() override;
	private:
		uint32_t m_Id;
	};
}

