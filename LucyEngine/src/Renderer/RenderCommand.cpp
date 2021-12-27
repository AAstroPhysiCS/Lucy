#include "lypch.h"
#include "RenderCommand.h"

#include "Renderer/Renderer.h"
#include "OpenGLRenderCommand.h"
#include "VulkanRenderCommand.h"

namespace Lucy {

	RefLucy<Pipeline> RenderCommand::s_ActivePipeline = nullptr;

	RefLucy<RenderCommand> RenderCommand::Create() {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLRenderCommand>();
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanRenderCommand>();
				break;
		}
	}
}