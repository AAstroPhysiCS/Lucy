#pragma once

#include "vulkan/vulkan.h"

namespace Lucy {

	enum class ImageFormat;

	struct ClearColor {
		float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
	};

	enum class RenderPassStoreOp {
		Store,
		DontCare,
		None
	};

	enum class RenderPassLoadOp {
		Load,
		Clear,
		DontCare,
		None
	};

	using RenderPassInternalLayout = uint32_t;
	
	struct RenderPassLayout {
		struct AttachmentReference {
			RenderPassInternalLayout Layout = 0x7FFFFFFF;
		};

		struct Attachment {
			ImageFormat Format;
			uint32_t Samples = 1;
			RenderPassLoadOp LoadOp = RenderPassLoadOp::None;
			RenderPassStoreOp StoreOp = RenderPassStoreOp::None;
			RenderPassLoadOp StencilLoadOp = RenderPassLoadOp::None;
			RenderPassStoreOp StencilStoreOp = RenderPassStoreOp::None;

			RenderPassInternalLayout Initial = 0x7FFFFFFF;
			RenderPassInternalLayout Final = 0x7FFFFFFF;

			AttachmentReference Reference;

			inline bool IsValid() const {
				return Initial != 0x7FFFFFFF && Final != 0x7FFFFFFF;
			}
		};

		std::vector<Attachment> ColorAttachments;
		Attachment DepthAttachment;
	};

	struct RenderPassCreateInfo {
		struct Multiview {
			/*
				Bit mask that specifies which view rendering is broadcast to
				For example; 0011 = Broadcast to first and second view (layer)
			*/
			uint32_t ViewMask = 0x7FFFFFFF;

			/*
				Bit mask that specifies correlation between views
				An implementation may use this for optimizations (concurrent render)
			*/
			uint32_t CorrelationMask = 0x7FFFFFFF;

			inline bool IsValid() const {
				return ViewMask != 0x7FFFFFFF && CorrelationMask != 0x7FFFFFFF;
			}
		};

		ClearColor ClearColor;
		RenderPassLayout Layout;
		Multiview Multiview;
	};

	class RenderPass {
	public:
		static Ref<RenderPass> Create(const RenderPassCreateInfo& createInfo);

		virtual void Recreate() = 0;
		virtual void Destroy() = 0;

		inline bool IsDepthBuffered() const { return m_DepthBuffered; }
		inline const RenderPassLayout& GetLayout() const { return m_CreateInfo.Layout; }

		RenderPass(const RenderPassCreateInfo& createInfo);
		virtual ~RenderPass() = default;

		inline ClearColor GetClearColor() { return m_CreateInfo.ClearColor; }
	protected:
		RenderPassCreateInfo m_CreateInfo;

		bool m_DepthBuffered = false;
	};

	uint32_t GetAPILoadOp(RenderPassLoadOp loadOp);
	uint32_t GetAPIStoreOp(RenderPassStoreOp storeOp);
}