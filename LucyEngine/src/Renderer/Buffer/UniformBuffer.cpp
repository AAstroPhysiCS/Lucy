#include "lypch.h"
#include "UniformBuffer.h"

#include "Renderer/Renderer.h"
#include "OpenGL/OpenGLUniformBuffer.h"

namespace Lucy {

	RefLucy<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLUniformBuffer>(size, binding);
				break;
			case RenderArchitecture::Vulkan:
				LUCY_CRITICAL("Vulkan not supported");
				LUCY_ASSERT(false);
				break;
		}
	}
}