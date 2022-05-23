#pragma once
#include "../Image/Image.h"
#include "../../Core/Base.h"

namespace Lucy {

	class RenderBuffer;
	class RenderPass;
	class VulkanImage2D;
	class VulkanImageView;

	struct OpenGLRHIFrameBufferDesc {
		bool DisableReadWriteBuffer = false;
		bool IsStorage = false;

		ImageSpecification BlittedTextureSpecs;

		RefLucy<RenderBuffer> RenderBuffer;
	};

	struct VulkanRHIFrameBufferDesc {
		std::vector<RefLucy<VulkanImage2D>> ImageBuffers;
		RefLucy<RenderPass> RenderPass = nullptr;

		//only for swapchain (use ImageBuffer)
		std::vector<VulkanImageView> ImageViews;
	};

	struct FrameBufferSpecification {
		bool MultiSampled = false;
		int32_t	Width = 0, Height = 0;
		int32_t Level = 0;

		std::vector<ImageSpecification> TextureSpecs;

		RefLucy<void> InternalInfo = nullptr; //to be overriden by different rhi's
	};

	class FrameBuffer {
	public:
		~FrameBuffer() = default;

		static RefLucy<FrameBuffer> Create(FrameBufferSpecification& specs);

		virtual void Bind() = 0;
		virtual void Unbind() = 0;
		virtual void Destroy() = 0;
	protected:
		FrameBuffer(FrameBufferSpecification& m_Specs);

		FrameBufferSpecification m_Specs;
	};
}
