#pragma once

#include "../VertexBuffer.h"

namespace Lucy {

	class OpenGLVertexBuffer : public VertexBuffer {
	public:
		OpenGLVertexBuffer(uint32_t size);
		OpenGLVertexBuffer();
		virtual ~OpenGLVertexBuffer() = default;

		void Bind(const VertexBindInfo& info);
		void Unbind();
		void AddData(const std::vector<float>& dataToAdd);
		void Load();
		void Destroy();
	};
}

