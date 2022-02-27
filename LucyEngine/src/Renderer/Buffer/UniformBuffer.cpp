#include "lypch.h"
#include "UniformBuffer.h"

#include "Renderer/Renderer.h"
#include "OpenGL/OpenGLUniformBuffer.h"
#include "Vulkan/VulkanUniformBuffer.h"

namespace Lucy {

	RefLucy<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding, std::optional<VulkanDescriptorSet> descriptorSet) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLUniformBuffer>(size, binding);
				break;
			case RenderArchitecture::Vulkan:
				LUCY_ASSERT(descriptorSet.has_value());
				return CreateRef<VulkanUniformBuffer>(size, binding, descriptorSet.value());
				break;
		}
		return nullptr;
	}
}