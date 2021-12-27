#pragma once

#include "../Texture/Texture.h"
#include "../../Core/Base.h"

#include "RenderBuffer.h"
#include "Renderer/VulkanRenderPass.h"

namespace Lucy {

	class RenderBuffer;

	struct TextureSpecification;

	struct FrameBufferSpecification {
		bool MultiSampled = false;
		bool DisableReadWriteBuffer = false;
		bool IsStorage = false;
		int32_t	ViewportWidth = 0, ViewportHeight = 0;
		int32_t Level = 0;

		std::vector<TextureSpecification> TextureSpecs;
		TextureSpecification BlittedTextureSpecs;

		RefLucy<RenderBuffer> RenderBuffer;

		//Vulkan only
	private:
		RefLucy<VulkanRenderPass> RenderPass;

		friend class VulkanFrameBuffer;
		friend class VulkanRenderer;
	};

	class FrameBuffer {
	public:
		~FrameBuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;
		virtual void Blit() = 0;
		virtual void Resize(int32_t width, int32_t height) = 0;

		uint32_t GetID() const { return m_Id; }
		RefLucy<FrameBuffer>& GetBlitted() { return m_Blitted; }

		inline auto GetSizeFromTexture(uint32_t index) const {
			struct Size { int32_t Width, Height; };
			return Size{ m_Specs.TextureSpecs[index].Width, m_Specs.TextureSpecs[index].Height };
		}

		static RefLucy<FrameBuffer> Create(FrameBufferSpecification& specs);
	protected:
		FrameBuffer(FrameBufferSpecification& specs);

		uint32_t m_Id;
		FrameBufferSpecification m_Specs;
		RefLucy<FrameBuffer> m_Blitted;
	};
}
