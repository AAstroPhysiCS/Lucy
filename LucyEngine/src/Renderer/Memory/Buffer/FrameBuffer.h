#pragma once
#include "Renderer/Image/Image.h"
#include "Core/Base.h"

namespace Lucy {

	class RenderBuffer;
	class RenderPass;
	class VulkanImage2D;
	class VulkanImageView;

	struct OpenGLRHIFrameBufferDesc {
		bool DisableReadWriteBuffer = false;
		bool IsStorage = false;
		
		std::vector<ImageSpecification> TextureSpecs;
		ImageSpecification BlittedTextureSpecs;

		Ref<RenderBuffer> RenderBuffer;
	};

	struct VulkanRHIFrameBufferDesc {
		std::vector<Ref<VulkanImage2D>> ImageBuffers;
		Ref<RenderPass> RenderPass = nullptr;

		//only for swapchain (use ImageBuffer)
		std::vector<VulkanImageView> ImageViews;
	};

	struct FrameBufferSpecification {
		bool MultiSampled = false;
		int32_t	Width = 0, Height = 0;
		int32_t Level = 1;

		Ref<void> InternalInfo = nullptr; //to be overriden by different rhi's
	};

	class FrameBuffer {
	public:
		virtual ~FrameBuffer() = default;

		static Ref<FrameBuffer> Create(FrameBufferSpecification& specs);
		virtual void Destroy() = 0;

		inline uint32_t GetWidth() { return m_Specs.Width; }
		inline uint32_t GetHeight() { return m_Specs.Height; }
	protected:
		FrameBuffer(FrameBufferSpecification& m_Specs);

		FrameBufferSpecification m_Specs;
	};
}