#pragma once

#include "../FrameBuffer.h"
#include "../../Image/OpenGLImage.h"

namespace Lucy {

	class OpenGLFrameBuffer : public FrameBuffer {
	public:
		OpenGLFrameBuffer(FrameBufferSpecification& specs);
		virtual ~OpenGLFrameBuffer() = default;

		void Bind() override;
		void Unbind() override;
		void Destroy() override;
		void Blit();
		void Resize(int32_t width, int32_t height);
		RefLucy<OpenGLFrameBuffer>& GetBlitted() { return m_Blitted; }

		inline RefLucy<OpenGLImage2D>& GetTexture(uint32_t index) { return m_Textures[index]; }

		inline auto GetSizeFromTexture(uint32_t index) const {
			struct Size { int32_t Width, Height; };
			return Size{ m_Specs.TextureSpecs[index].Width, m_Specs.TextureSpecs[index].Height };
		}

		uint32_t GetID() const { return m_Id; }
	private:
		bool CheckStatus();
		std::vector<RefLucy<OpenGLImage2D>> m_Textures;
		RefLucy<OpenGLFrameBuffer> m_Blitted;

		uint32_t m_Id = 0;
		FrameBufferSpecification m_Specs;
	};
}

