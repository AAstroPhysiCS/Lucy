#pragma once

#include "Renderer/Image/Image.h" //for ImageCreateInfo

namespace Lucy {

	class RenderPass;
	class VulkanImageView;

	struct VulkanFrameBufferInfo {
		Ref<RenderPass> RenderPass = nullptr;

		//only for swapchain (use ImageBuffer)
		std::vector<VulkanImageView> ImageViews;
	};

	struct FrameBufferCreateInfo {
		bool MultiSampled = false;
		int32_t	Width = 0, Height = 0;
		int32_t Level = 1;
		
		std::vector<Ref<Image2D>> ImageBuffers;

		Ref<void> InternalInfo = nullptr; //to be overriden by different api's
	};

	class FrameBuffer {
	public:
		virtual ~FrameBuffer() = default;

		static Ref<FrameBuffer> Create(FrameBufferCreateInfo& createInfo);
		virtual void Recreate(uint32_t width, uint32_t height, Ref<void> internalInfo = nullptr) = 0;
		virtual void Destroy() = 0;

		inline uint32_t GetWidth() { return m_CreateInfo.Width; }
		inline uint32_t GetHeight() { return m_CreateInfo.Height; }
	protected:
		FrameBuffer(FrameBufferCreateInfo& m_CreateInfo);

		FrameBufferCreateInfo m_CreateInfo;
	};
}