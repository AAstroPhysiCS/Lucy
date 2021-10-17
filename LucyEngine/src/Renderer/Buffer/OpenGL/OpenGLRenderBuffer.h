#pragma once

#include "../RenderBuffer.h"

namespace Lucy {

	class OpenGLRenderBuffer : public RenderBuffer
	{
	public:
		OpenGLRenderBuffer(RenderBufferSpecification& specs);
		virtual ~OpenGLRenderBuffer() = default;

		void Bind();
		void Unbind();
		void AttachToFramebuffer();

		inline uint32_t GetInternalFormat() const { return m_Specs.internalFormat; }
		inline uint32_t GetAttachment() const { return m_Specs.attachment; }
	};
}
