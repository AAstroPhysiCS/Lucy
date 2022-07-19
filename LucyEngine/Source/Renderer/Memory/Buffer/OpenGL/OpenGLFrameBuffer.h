#pragma once

#include "../FrameBuffer.h"
#include "Renderer/Image/OpenGLImage.h"

namespace Lucy {

	class OpenGLFrameBuffer : public FrameBuffer {
	public:
		OpenGLFrameBuffer(FrameBufferCreateInfo& createInfo);
		virtual ~OpenGLFrameBuffer() = default;

		void Bind();
		void Unbind();
		void Destroy() override;
		void Blit();
		void Resize(int32_t width, int32_t height);
		Ref<OpenGLFrameBuffer>& GetBlitted() { return m_Blitted; }

		inline Ref<OpenGLImage2D>& GetTexture(uint32_t index) { return m_Textures[index]; }

		inline auto GetSizeFromTexture(uint32_t index) const {
			struct Size { int32_t Width, Height; };
			auto& desc = m_CreateInfo.InternalInfo.As<OpenGLRHIFrameBufferDesc>();
			return Size{ desc->TextureCreateInfos[index].Width, desc->TextureCreateInfos[index].Height };
		}

		uint32_t GetID() const { return m_Id; }
	private:
		bool CheckStatus();
		std::vector<Ref<OpenGLImage2D>> m_Textures;
		Ref<OpenGLFrameBuffer> m_Blitted;

		uint32_t m_Id = 0;
	};
}

