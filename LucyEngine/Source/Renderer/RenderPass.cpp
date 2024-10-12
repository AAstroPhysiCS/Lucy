#include "lypch.h"
#include "RenderPass.h"
#include "VulkanRenderPass.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	RenderPass::RenderPass(const RenderPassCreateInfo& createInfo)
		: RenderResource("RenderPass"), m_CreateInfo(createInfo) {
	}

	std::array<uint32_t, 2> GetAPILoadStoreAttachments(RenderPassLoadStoreAttachments loadStoreOp) {
		std::array<uint32_t, 2> result = { 0, 0 };

		enum class RenderPassLoadOp {
			Clear,
			DontCare,
			None,
			Load
		};

		enum class RenderPassStoreOp {
			DontCare,
			None,
			Store
		};

		//TODO: For D3D12
		const auto GetAPILoadOp = [](RenderPassLoadOp loadOp) {
			if (Renderer::GetRenderArchitecture() != RenderArchitecture::Vulkan)
				LUCY_ASSERT(false);

			switch (loadOp) {
				case RenderPassLoadOp::Clear:
					return VK_ATTACHMENT_LOAD_OP_CLEAR;
				case RenderPassLoadOp::DontCare:
					return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				case RenderPassLoadOp::None:
					return VK_ATTACHMENT_LOAD_OP_NONE_EXT;
				case RenderPassLoadOp::Load:
					return VK_ATTACHMENT_LOAD_OP_LOAD;
				default:
					return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
			}
		};

		const auto GetAPIStoreOp = [](RenderPassStoreOp storeOp) {
			if (Renderer::GetRenderArchitecture() != RenderArchitecture::Vulkan)
				LUCY_ASSERT(false);

			switch (storeOp) {
				case RenderPassStoreOp::DontCare:
					return VK_ATTACHMENT_STORE_OP_DONT_CARE;
				case RenderPassStoreOp::None:
					return VK_ATTACHMENT_STORE_OP_NONE_EXT;
				case RenderPassStoreOp::Store:
					return VK_ATTACHMENT_STORE_OP_STORE;
				default:
					return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
			}
		};

		switch (loadStoreOp) {
			case RenderPassLoadStoreAttachments::NoneNone: {
				result[0] = GetAPILoadOp(RenderPassLoadOp::None);
				result[1] = GetAPIStoreOp(RenderPassStoreOp::None);
				break;
			}
			case RenderPassLoadStoreAttachments::NoneDontCare: {
				result[0] = GetAPILoadOp(RenderPassLoadOp::None);
				result[1] = GetAPIStoreOp(RenderPassStoreOp::DontCare);
				break;
			}
			case RenderPassLoadStoreAttachments::NoneStore: {
				result[0] = GetAPILoadOp(RenderPassLoadOp::None);
				result[1] = GetAPIStoreOp(RenderPassStoreOp::Store);
				break;
			}
			case RenderPassLoadStoreAttachments::ClearNone: {
				result[0] = GetAPILoadOp(RenderPassLoadOp::Clear);
				result[1] = GetAPIStoreOp(RenderPassStoreOp::None);
				break;
			}
			case RenderPassLoadStoreAttachments::ClearDontCare: {
				result[0] = GetAPILoadOp(RenderPassLoadOp::Clear);
				result[1] = GetAPIStoreOp(RenderPassStoreOp::DontCare);
				break;
			}
			case RenderPassLoadStoreAttachments::ClearStore: {
				result[0] = GetAPILoadOp(RenderPassLoadOp::Clear);
				result[1] = GetAPIStoreOp(RenderPassStoreOp::Store);
				break;
			}
			case RenderPassLoadStoreAttachments::DontCareNone: {
				result[0] = GetAPILoadOp(RenderPassLoadOp::DontCare);
				result[1] = GetAPIStoreOp(RenderPassStoreOp::None);
				break;
			}
			case RenderPassLoadStoreAttachments::DontCareDontCare: {
				result[0] = GetAPILoadOp(RenderPassLoadOp::DontCare);
				result[1] = GetAPIStoreOp(RenderPassStoreOp::DontCare);
				break;
			}
			case RenderPassLoadStoreAttachments::DontCareStore: {
				result[0] = GetAPILoadOp(RenderPassLoadOp::DontCare);
				result[1] = GetAPIStoreOp(RenderPassStoreOp::Store);
				break;
			}
			case RenderPassLoadStoreAttachments::LoadNone: {
				result[0] = GetAPILoadOp(RenderPassLoadOp::Load);
				result[1] = GetAPIStoreOp(RenderPassStoreOp::None);
				break;
			}
			case RenderPassLoadStoreAttachments::LoadStore: {
				result[0] = GetAPILoadOp(RenderPassLoadOp::Load);
				result[1] = GetAPIStoreOp(RenderPassStoreOp::Store);
				break;
			}
			case RenderPassLoadStoreAttachments::LoadDontCare: {
				result[0] = GetAPILoadOp(RenderPassLoadOp::Load);
				result[1] = GetAPIStoreOp(RenderPassStoreOp::DontCare);
				break;
			}
		}

		return result;
	}
}