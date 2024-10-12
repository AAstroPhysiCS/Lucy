#pragma once

#include "Renderer/Device/RenderResource.h"

namespace Lucy {

	class RenderPass;
	class VulkanImageView;
	class Image;

	struct FrameBufferCreateInfo {
		uint32_t Width = 0, Height = 0;
		bool MultiSampled = false;
		bool IsInFlight = false;
		
		RenderResourceHandle RenderPassHandle = InvalidRenderResourceHandle;
		std::vector<RenderResourceHandle> ImageBufferHandles;

		RenderResourceHandle DepthImageHandle = InvalidRenderResourceHandle;
	};

	class FrameBuffer : public RenderResource {
	public:
		virtual ~FrameBuffer() = default;

		virtual void RTRecreate(uint32_t width, uint32_t height) = 0;

		inline uint32_t GetWidth() const { return m_CreateInfo.Width; }
		inline uint32_t GetHeight() const { return m_CreateInfo.Height; }
	protected:
		FrameBuffer(const FrameBufferCreateInfo& createInfo)
			: RenderResource("FrameBuffer"), m_CreateInfo(createInfo) {
		}
		FrameBufferCreateInfo m_CreateInfo;
	};
}