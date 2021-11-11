#pragma once

#include "../../Core/Base.h"

namespace Lucy {

	class Renderer;

	struct RenderBufferSpecification {
		uint32_t Width, Height;
		uint32_t InternalFormat, Attachment;
		uint32_t Samples;
	};

	class RenderBuffer {
	public:
		static RefLucy<RenderBuffer> Create(RenderBufferSpecification& specs);

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		inline uint32_t GetID() const { return m_Id; }
		inline uint32_t GetWidth() const { return m_Specs.Width; }
		inline uint32_t GetHeight() const { return m_Specs.Height; }
	protected:
		RenderBuffer(RenderBufferSpecification& specs);
		virtual ~RenderBuffer() = default;

		RenderBufferSpecification m_Specs;
		uint32_t m_Id = 0;
	};
}
