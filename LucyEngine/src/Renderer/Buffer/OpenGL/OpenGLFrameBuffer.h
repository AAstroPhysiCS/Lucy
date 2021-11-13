#pragma once

#include "../FrameBuffer.h"
#include "../../Texture/Texture.h"

namespace Lucy {

	class OpenGLFrameBuffer : public FrameBuffer {
	public:
		OpenGLFrameBuffer(FrameBufferSpecification& specs);
		virtual ~OpenGLFrameBuffer() = default;

		void Bind();
		void Unbind();
		void Destroy();
		void Blit();
		void Resize(int32_t width, int32_t height);

		inline RefLucy<Texture2D>& GetTexture(uint32_t index) { return m_Textures[index]; }
	private:
		bool CheckStatus();
		std::vector<RefLucy<Texture2D>> m_Textures;
	};
}

