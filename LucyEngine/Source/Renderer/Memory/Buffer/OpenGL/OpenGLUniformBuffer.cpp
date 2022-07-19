#include "lypch.h"
#include "OpenGLUniformBuffer.h"

#include "Renderer/Renderer.h"
#include "Renderer/Descriptors/DescriptorSet.h"

#include "glad/glad.h"

namespace Lucy {

	OpenGLUniformBuffer::OpenGLUniformBuffer(UniformBufferCreateInfo& createInfo)
		: UniformBuffer(createInfo) {
		Renderer::Enqueue([=]() {
			glCreateBuffers(1, &m_Id);
			glNamedBufferData(m_Id, m_CreateInfo.BufferSize, nullptr, GL_DYNAMIC_DRAW);
			glBindBufferBase(GL_UNIFORM_BUFFER, m_CreateInfo.Binding, m_Id);
		});
	}

	void OpenGLUniformBuffer::Bind() {
		glBindBuffer(GL_UNIFORM_BUFFER, m_Id);
	}

	void OpenGLUniformBuffer::Unbind() {
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void OpenGLUniformBuffer::LoadToGPU() {

	}

	void OpenGLUniformBuffer::DestroyHandle() {
		glDeleteBuffers(1, &m_Id);
	}

	void OpenGLUniformBuffer::SetData(void* data, uint32_t size, uint32_t offset) {
		glNamedBufferSubData(m_Id, offset, size, data);
	}
}