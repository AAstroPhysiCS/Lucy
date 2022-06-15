#include "lypch.h"
#include "IndexBuffer.h"

#include "OpenGL/OpenGLIndexBuffer.h"
#include "Vulkan/VulkanIndexBuffer.h"

#include "../Renderer.h"

namespace Lucy {

	RefLucy<IndexBuffer> IndexBuffer::Create(uint32_t size) {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLIndexBuffer>(size);
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanIndexBuffer>(size);
				break;
		}
		return nullptr;
	}

	RefLucy<IndexBuffer> IndexBuffer::Create() {
		switch (Renderer::GetCurrentRenderArchitecture()) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLIndexBuffer>();
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanIndexBuffer>();
				break;
		}
		return nullptr;
	}

	IndexBuffer::IndexBuffer(uint32_t size) {
		Allocate(size); //internal std::vector allocation
	}
}