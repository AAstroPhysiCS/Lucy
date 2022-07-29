#include "lypch.h"

#include "RenderContext.h"
#include "VulkanContext.h"

namespace Lucy {

	Ref<RenderContext> RenderContext::Create(RenderArchitecture arch, Ref<Window>& window) {
		switch (arch) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanContext>(window);
				break;
			default:
				LUCY_CRITICAL("Other API's are not supported!");
				LUCY_ASSERT(false);
				break;
		}
		return nullptr;
	}

	RenderContext::RenderContext(Ref<Window>& window)
		: m_Window(window) {
	}
}