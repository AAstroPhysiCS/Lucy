#pragma once

#include "../VertexBuffer.h"

namespace Lucy {

	class OpenGLVertexBuffer : public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(uint32_t size);
		OpenGLVertexBuffer() = default;
		virtual ~OpenGLVertexBuffer() = default;

		void Bind();
		void Unbind();
		void AddData(std::vector<float>& dataToAdd);
		void Load();
		void Destroy();
	private:
		uint32_t m_DynamicOffset = 0;
	};
}

