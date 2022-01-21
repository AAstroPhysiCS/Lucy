#pragma once

#include "../IndexBuffer.h"

namespace Lucy {

	class OpenGLIndexBuffer : public IndexBuffer {
	public:
		OpenGLIndexBuffer(uint32_t size);
		OpenGLIndexBuffer();
		virtual ~OpenGLIndexBuffer() = default;

		void Bind(const IndexBindInfo& info);
		void Unbind();
		void AddData(const std::vector<uint32_t>& dataToAdd);
		void Load();
		void Destroy();
	};
}