#include "lypch.h"
#include "RenderCommand.h"

#include "Renderer/Renderer.h"
#include "OpenGLRenderCommand.h"

namespace Lucy {

	RenderPass* RenderCommand::s_ActiveRenderPass = nullptr;

	RefLucy<RenderCommand> RenderCommand::Create() {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLRenderCommand>();
				break;
			case RenderArchitecture::Vulkan:
				LUCY_CRITICAL("Vulkan not supported");
				LUCY_ASSERT(false);
				break;
		}
	}
}