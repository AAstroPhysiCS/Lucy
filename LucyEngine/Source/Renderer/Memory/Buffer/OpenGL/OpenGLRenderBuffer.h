#pragma once

#include "../RenderBuffer.h"

namespace Lucy {

	class OpenGLRenderBuffer : public RenderBuffer {
	public:
		OpenGLRenderBuffer(const RenderBufferCreateInfo& createInfo);
		virtual ~OpenGLRenderBuffer() = default;

		void Bind() override;
		void Unbind() override;
		void Resize(int32_t width, int32_t height) override;
		void Destroy() override;
		void AttachToFramebuffer();

		inline uint32_t GetInternalFormat() const { return m_CreateInfo.InternalFormat; }
		inline uint32_t GetAttachment() const { return m_CreateInfo.Attachment; }
	};
}
