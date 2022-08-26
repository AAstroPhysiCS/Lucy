#include "lypch.h"
#include "RenderPass.h"
#include "VulkanRenderPass.h"

#include "Renderer/Renderer.h"

namespace Lucy {

	RenderPass::RenderPass(const RenderPassCreateInfo& createInfo)
		: m_CreateInfo(createInfo), m_DepthBuffered(createInfo.DepthEnable) {
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
}