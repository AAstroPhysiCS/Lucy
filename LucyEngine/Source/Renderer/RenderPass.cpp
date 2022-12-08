#include "lypch.h"
#include "RenderPass.h"
#include "VulkanRenderPass.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	RenderPass::RenderPass(const RenderPassCreateInfo& createInfo)
		: m_CreateInfo(createInfo) {
	}

	Ref<RenderPass> Lucy::RenderPass::Create(const RenderPassCreateInfo& createInfo) {
		switch (Renderer::GetRenderArchitecture()) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanRenderPass>(createInfo);
				break;
			default:
				LUCY_CRITICAL("Other API's are not supported!");
				LUCY_ASSERT(false);
				break;
		}
		return nullptr;
	}

	uint32_t GetAPILoadOp(RenderPassLoadOp loadOp) {
		if (Renderer::GetRenderArchitecture() != RenderArchitecture::Vulkan) {
			LUCY_ASSERT(false);
			return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
		}
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
	}

	uint32_t GetAPIStoreOp(RenderPassStoreOp storeOp) {
		if (Renderer::GetRenderArchitecture() != RenderArchitecture::Vulkan) {
			LUCY_ASSERT(false);
			return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
		}
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
	}
}