#pragma once

#include "Buffer.h"
#include "../../Core/Base.h"

namespace Lucy {

	struct TextureSpecification;

	struct FrameBufferSpecification {

		bool multiSampled;
		
		uint32_t texelDataType = 0;
		uint32_t paramMinFilter = -1, paramMagFilter = -1;
		uint32_t wrapS = -1, wrapT = -1, wrapR = -1;

		bool disableReadWriteBuffer;
		bool isStorage;
		uint32_t level = -1;

		uint32_t textureSize;
		TextureSpecification* textureSpecs;

		//RenderBuffer* renderBuffer;
	};

	class FrameBuffer
	{
	public:
		~FrameBuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;
		
		static RefLucy<FrameBuffer> Create(FrameBufferSpecification& specs);
	protected:
		FrameBuffer(FrameBufferSpecification& specs);
		uint32_t m_Id;

	private:
		FrameBufferSpecification m_Specs;
	};
}
