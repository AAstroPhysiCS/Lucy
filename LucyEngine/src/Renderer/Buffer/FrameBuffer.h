#pragma once
#include "../Texture/Texture.h"
#include "../../Core/Base.h"

#include "RenderBuffer.h"

namespace Lucy {

	class RenderBuffer;
	class RenderPass;
	class VulkanImageView;

	struct TextureSpecification;

	//TODO: More generalization is needed here!
	struct FrameBufferSpecification {
		bool MultiSampled = false;
		bool DisableReadWriteBuffer = false;
		bool IsStorage = false;
		int32_t	Width = 0, Height = 0;
		int32_t Level = 0;

		std::vector<TextureSpecification> TextureSpecs;
		TextureSpecification BlittedTextureSpecs;

		RefLucy<RenderBuffer> RenderBuffer;

		//Vulkan stuff, should be changed TODO:
		std::vector<VulkanImageView> ImageViews;
	};

	class FrameBuffer {
	public:
		~FrameBuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;

		uint32_t GetID() const { return m_Id; }

		inline auto GetSizeFromTexture(uint32_t index) const {
			struct Size { int32_t Width, Height; };
			return Size{ m_Specs.TextureSpecs[index].Width, m_Specs.TextureSpecs[index].Height };
		}

		static RefLucy<FrameBuffer> Create(FrameBufferSpecification& specs, RefLucy<RenderPass> renderPass = nullptr);
	protected:
		FrameBuffer(FrameBufferSpecification& specs);

		uint32_t m_Id = 0;
		FrameBufferSpecification m_Specs;
	};
}
