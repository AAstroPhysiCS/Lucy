#include "lypch.h"
#include "UniformBuffer.h"

#include "Renderer/Renderer.h"
#include "OpenGL/OpenGLUniformBuffer.h"
#include "Vulkan/VulkanUniformBuffer.h"

namespace Lucy {

	RefLucy<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLUniformBuffer>(size, binding);
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanUniformBuffer>(size, binding);
				break;
		}
	}
}