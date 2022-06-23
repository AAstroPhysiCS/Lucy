#include "lypch.h"
#include "RenderPass.h"

#include "OpenGLRenderPass.h"
#include "VulkanRenderPass.h"
#include "Renderer.h"

namespace Lucy {

	RenderPass::RenderPass(const RenderPassSpecification& specs)
		: m_Specs(specs) {
	}

	Ref<RenderPass> Lucy::RenderPass::Create(const RenderPassSpecification& specs) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return Memory::CreateRef<OpenGLRenderPass>(specs);
				break;
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanRenderPass>(specs);
				break;
			default:
				LUCY_CRITICAL("Other API's are not supported!");
				LUCY_ASSERT(false);
				break;
		}
		return nullptr;
	}
}