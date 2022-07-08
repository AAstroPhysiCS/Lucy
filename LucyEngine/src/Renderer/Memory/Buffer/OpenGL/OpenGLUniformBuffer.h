#pragma once

#include "../UniformBuffer.h"

namespace Lucy {

	class OpenGLUniformBuffer : public UniformBuffer {
	public:
		OpenGLUniformBuffer(UniformBufferCreateInfo& createInfo);
		virtual ~OpenGLUniformBuffer() = default;

		void Bind();
		void Unbind();
		void Update() override;
		void DestroyHandle() override;
		void SetData(void* data, uint32_t size, uint32_t offset);
	private:
		uint32_t m_Id;
	};
}

