#pragma once

#include "../../Core/Base.h"
#include <cstdint>

namespace Lucy {

	class Renderer;

	struct RenderBufferSpecification {
		uint32_t width, height;
		uint32_t internalFormat, attachment;
		uint32_t samples;
	};

	class RenderBuffer
	{
	public:
		static RefLucy<RenderBuffer> Create(RenderBufferSpecification& specs);

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		inline uint32_t GetID() const { return m_Id; }
		inline uint32_t GetWidth() const { return m_Specs.width; }
		inline uint32_t GetHeight() const { return m_Specs.height; }
	protected:
		RenderBuffer(RenderBufferSpecification& specs);
		~RenderBuffer() = default;

		RenderBufferSpecification m_Specs;
		uint32_t m_Id = 0;
	};
}
