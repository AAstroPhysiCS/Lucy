#pragma once

#include "../IndexBuffer.h"

namespace Lucy {

	class OpenGLIndexBuffer : public IndexBuffer {
	public:
		OpenGLIndexBuffer(uint32_t size);
		OpenGLIndexBuffer();
		virtual ~OpenGLIndexBuffer() = default;

		void Bind();
		void Unbind();
		void AddData(std::vector<uint32_t>& dataToAdd);
		void Load();
		void Destroy();
	};
}