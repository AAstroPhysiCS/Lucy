#include "lypch.h"

#include "RenderContext.h"
#include "OpenGLContext.h"
#include "VulkanContext.h"

namespace Lucy {

	Ref<RenderContext> RenderContext::Create(RenderArchitecture arch) {
		switch (arch) {
			case RenderArchitecture::OpenGL:
				return Memory::CreateRef<OpenGLContext>();
				break;
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanContext>();
				break;
			default:
				LUCY_CRITICAL("Other API's are not supported!");
				LUCY_ASSERT(false);
				break;
		}
		return nullptr;
	}
}