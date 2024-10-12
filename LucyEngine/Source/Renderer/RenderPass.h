#pragma once

#include "vulkan/vulkan.h"

#include "Renderer/Device/RenderResource.h"

namespace Lucy {

	enum class ImageFormat;

	struct ClearColor {
		float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
	};

	enum class RenderPassLoadStoreAttachments : uint8_t {
		NoneNone,
		NoneDontCare,
		NoneStore,

		ClearNone,
		ClearDontCare,
		ClearStore,

		LoadNone,
		LoadDontCare,
		LoadStore,

		DontCareNone,
		DontCareDontCare,
		DontCareStore
	};

	using RenderPassInternalLayout = uint32_t;
	
	struct RenderPassLayout {
		struct AttachmentReference {
			RenderPassInternalLayout Layout = 0x7FFFFFFF;
		};

		struct Attachment {
			ImageFormat Format;
			uint32_t Samples = 1;
			RenderPassLoadStoreAttachments LoadStoreOperation;
			RenderPassLoadStoreAttachments StencilLoadStoreOperation;

			RenderPassInternalLayout Initial = 0x7FFFFFFF;
			RenderPassInternalLayout Final = 0x7FFFFFFF;

			AttachmentReference Reference;

			inline bool IsValid() const {
				return Initial != 0x7FFFFFFF && Final != 0x7FFFFFFF;
			}
		};

		using Attachments = std::vector<Attachment>;

		Attachments ColorAttachments;
		Attachment DepthAttachment;
	};

	struct RenderPassCreateInfo {
		struct Multiview {
			/*
				Bit mask that specifies which view rendering is broadcast to
				For example; 0011 = Broadcast to first and second view (layer)
			*/
			uint32_t ViewMask = 0x7FFFFFFFu;

			/*
				Bit mask that specifies correlation between views
				An implementation may use this for optimizations (concurrent render)
			*/
			uint32_t CorrelationMask = 0x7FFFFFFFu;

			inline bool IsValid() const {
				return ViewMask != 0x7FFFFFFFu && CorrelationMask != 0x7FFFFFFFu;
			}
		};

		ClearColor ClearColor;
		RenderPassLayout Layout;
		Multiview Multiview;
	};

	class RenderPass : public RenderResource {
	public:
		virtual void RTRecreate() = 0;

		inline bool IsDepthBuffered() const { return m_DepthBuffered; }
		inline const RenderPassLayout& GetLayout() const { return m_CreateInfo.Layout; }

		RenderPass(const RenderPassCreateInfo& createInfo);
		virtual ~RenderPass() = default;

		inline ClearColor GetClearColor() { return m_CreateInfo.ClearColor; }
	protected:
		RenderPassCreateInfo m_CreateInfo;

		bool m_DepthBuffered = false;
	};

	std::array<uint32_t, 2> GetAPILoadStoreAttachments(RenderPassLoadStoreAttachments loadStoreOp);
}