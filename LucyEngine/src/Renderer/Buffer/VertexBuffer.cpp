#include "lypch.h"
#include "VertexBuffer.h"

#include "OpenGL/OpenGLVertexBuffer.h"
#include "Vulkan/VulkanVertexBuffer.h"

#include "../Renderer.h"

namespace Lucy {

	RefLucy<VertexBuffer> VertexBuffer::Create(uint32_t size) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLVertexBuffer>(size);
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanVertexBuffer>(size);
				break;
		}
		return nullptr;
	}

	RefLucy<VertexBuffer> VertexBuffer::Create() {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLVertexBuffer>();
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanVertexBuffer>();
				break;
		}
		return nullptr;
	}

	VertexBuffer::VertexBuffer(uint32_t size) {
		Allocate(size); //internal std::vector allocation
	}
}