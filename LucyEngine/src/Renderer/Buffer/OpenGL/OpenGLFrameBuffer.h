#pragma once

#include "../FrameBuffer.h"

namespace Lucy {
	class OpenGLFrameBuffer : public FrameBuffer
	{
	public:
		OpenGLFrameBuffer(FrameBufferSpecification& specs);
		virtual ~OpenGLFrameBuffer() = default;

		void Bind();
		void Unbind();
		void Destroy();
	private:
		bool CheckStatus();
	};
}

