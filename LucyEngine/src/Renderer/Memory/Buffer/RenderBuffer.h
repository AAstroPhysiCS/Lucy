#pragma once

#include "Core/Base.h"

namespace Lucy {

	struct RenderBufferCreateInfo {
		uint32_t Width = 0, Height = 0;
		uint32_t InternalFormat = 0, Attachment = 0;
		uint32_t Samples = 0;
	};

	class RenderBuffer {
	protected:
		RenderBuffer(const RenderBufferCreateInfo& createInfo);
	public:
		virtual ~RenderBuffer() = default;

		static Ref<RenderBuffer> Create(const RenderBufferCreateInfo& createInfo);

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Resize(int32_t width, int32_t height) = 0;
		virtual void Destroy() = 0;

		inline uint32_t GetID() const { return m_Id; }
		inline uint32_t GetWidth() const { return m_CreateInfo.Width; }
		inline uint32_t GetHeight() const { return m_CreateInfo.Height; }
	protected:
		RenderBufferCreateInfo m_CreateInfo;
		uint32_t m_Id = 0;
	};
}
