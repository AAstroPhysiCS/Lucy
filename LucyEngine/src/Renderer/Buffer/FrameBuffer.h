#pragma once

#include "../Texture/Texture.h"
#include "../../Core/Base.h"

namespace Lucy {

	class RenderBuffer;

	struct TextureSpecification;

	struct FrameBufferSpecification {
		bool MultiSampled;
		bool DisableReadWriteBuffer;
		bool IsStorage;
		int32_t Level = 0;

		std::vector<TextureSpecification> TextureSpecs;
		TextureSpecification BlittedTextureSpecs;

		RefLucy<RenderBuffer> RenderBuffer;
	};

	class FrameBuffer {
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
