#include "lypch.h"

#include "RenderContext.h"
#include "OpenGLContext.h"
#include "VulkanContext.h"

namespace Lucy {

	RenderContext::RenderContext(RenderArchitecture type)
	{}

	RefLucy<RenderContext> RenderContext::Create(RenderArchitecture type) {
		switch (type) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLContext>(type);
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanContext>(type);
				break;
			default:
				LUCY_CRITICAL("Other API's are not supported!");
				LUCY_ASSERT(false);
				break;
		}
	}
}