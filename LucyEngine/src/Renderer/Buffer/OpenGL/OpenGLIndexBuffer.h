#pragma once

#include "../IndexBuffer.h"

namespace Lucy {

	class OpenGLIndexBuffer : public IndexBuffer {
	public:
		explicit OpenGLIndexBuffer(uint32_t size);
		OpenGLIndexBuffer();
		virtual ~OpenGLIndexBuffer() = default;

		void Bind(const IndexBindInfo& info) override;
		void Unbind() override;
		void AddData(const std::vector<uint32_t>& dataToAdd) override;
		void Load() override;
		void Destroy() override;
	};
}