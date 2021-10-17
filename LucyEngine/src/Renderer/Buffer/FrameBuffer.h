#pragma once

#include "../../Core/Base.h"
#include "Buffer.h"
#include "../Texture/Texture.h"

namespace Lucy {

	class RenderBuffer;

	struct TextureSpecification;

	struct FrameBufferSpecification {
		bool multiSampled;
		bool disableReadWriteBuffer;
		bool isStorage;
		int32_t level = 0;

		std::vector<TextureSpecification> textureSpecs;
		TextureSpecification blittedTextureSpecs;

		RefLucy<RenderBuffer> renderBuffer;
	};

	class FrameBuffer
	{
	public:
		~FrameBuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;
		virtual void Blit() = 0;

		uint32_t GetID() const { return m_Id; }
		RefLucy<FrameBuffer>& GetBlitted() { return m_Blitted; }
		
		static RefLucy<FrameBuffer> Create(FrameBufferSpecification& specs);
	protected:
		FrameBuffer(FrameBufferSpecification& specs);
		
		uint32_t m_Id;
		FrameBufferSpecification m_Specs;
		RefLucy<FrameBuffer> m_Blitted;
	};
}
