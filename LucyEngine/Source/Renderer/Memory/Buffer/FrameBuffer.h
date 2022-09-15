#pragma once

namespace Lucy {

	class RenderPass;
	class VulkanImageView;
	class Image;

	struct FrameBufferCreateInfo {
		
		int32_t	Width = 0, Height = 0;
		bool MultiSampled = false;
		bool IsInFlight = false;
		
		Ref<RenderPass> RenderPass = nullptr;
		std::vector<Ref<Image>> ImageBuffers;

		Ref<Image> DepthImage = nullptr;

		//Vulkan only: only for swapchain (use ImageBuffer)
		std::vector<VulkanImageView> ImageViews;
	};

	class FrameBuffer {
	public:
		virtual ~FrameBuffer() = default;

		static Ref<FrameBuffer> Create(FrameBufferCreateInfo& createInfo);
		virtual void Recreate(uint32_t width, uint32_t height) = 0;
		virtual void Destroy() = 0;

		inline uint32_t GetWidth() { return m_CreateInfo.Width; }
		inline uint32_t GetHeight() { return m_CreateInfo.Height; }
	protected:
		FrameBuffer(FrameBufferCreateInfo& m_CreateInfo);

		FrameBufferCreateInfo m_CreateInfo;
	};
}