#pragma once

#include "../VertexBuffer.h"

namespace Lucy {

	class OpenGLVertexBuffer : public VertexBuffer {
	public:
		explicit OpenGLVertexBuffer(uint32_t size);
		OpenGLVertexBuffer();
		virtual ~OpenGLVertexBuffer() = default;

		void Bind(const VertexBindInfo& info) override;
		void Unbind() override;
		void AddData(const std::vector<float>& dataToAdd) override;
		void Load() override;
		void Destroy() override;
	};
}

