#pragma once

#include "../Renderer.h"
#include "Buffer.h"

namespace Lucy {

	struct FrameBufferSpecification {

	};

	class FrameBuffer
	{
	public:
		~FrameBuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		
		static RefLucy<FrameBuffer> Create(FrameBufferSpecification& specs);

	protected:
		FrameBuffer(FrameBufferSpecification& specs);

	private:
		FrameBufferSpecification m_Specs;
	};
}
