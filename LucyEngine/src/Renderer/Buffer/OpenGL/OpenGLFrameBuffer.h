#pragma once

#include "../FrameBuffer.h"

namespace Lucy {
	class OpenGLFrameBuffer : public FrameBuffer
	{
	public:
		OpenGLFrameBuffer(FrameBufferSpecification& specs);

		void Bind();
		void Unbind();
	};
}

