#pragma once

#include "Core/Base.h"

namespace Lucy {

	class Renderer;

	struct RenderBufferSpecification {
		uint32_t Width = 0, Height = 0;
		uint32_t InternalFormat = 0, Attachment = 0;
		uint32_t Samples = 0;
	};

	class RenderBuffer {
	protected:
		RenderBuffer(const RenderBufferSpecification& specs);
	public:
		virtual ~RenderBuffer() = default;

		static Ref<RenderBuffer> Create(const RenderBufferSpecification& specs);

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Resize(int32_t width, int32_t height) = 0;
		virtual void Destroy() = 0;

		inline uint32_t GetID() const { return m_Id; }
		inline uint32_t GetWidth() const { return m_Specs.Width; }
		inline uint32_t GetHeight() const { return m_Specs.Height; }
	protected:
		RenderBufferSpecification m_Specs;
		uint32_t m_Id = 0;
	};
}
