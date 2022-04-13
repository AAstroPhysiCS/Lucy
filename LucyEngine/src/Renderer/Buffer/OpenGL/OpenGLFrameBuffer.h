#pragma once

#include "../FrameBuffer.h"
#include "../../Texture/Texture.h"

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

		inline RefLucy<Texture2D>& GetTexture(uint32_t index) { return m_Textures[index]; }
	private:
		bool CheckStatus();
		std::vector<RefLucy<Texture2D>> m_Textures;
		RefLucy<OpenGLFrameBuffer> m_Blitted;
	};
}

