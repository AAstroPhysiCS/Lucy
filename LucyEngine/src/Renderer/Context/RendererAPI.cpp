#include "lypch.h"
#include "RendererAPI.h"

#include "Renderer/OpenGLRenderer.h"
#include "Renderer/VulkanRenderer.h"

namespace Lucy {

	RefLucy<RendererAPI> RendererAPI::Create(RenderArchitecture architecture) {
		switch (architecture) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLRenderer>(architecture);
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanRenderer>(architecture);
				break;
		}
	}
	
	RendererAPI::RendererAPI(RenderArchitecture renderArchitecture) {
		m_Architecture = renderArchitecture;
	}

	void RendererAPI::Init() {
		m_CameraUniformBuffer = UniformBuffer::Create(sizeof(glm::mat4) * 3, 0, {});
		m_TextureSlotsUniformBuffer = UniformBuffer::Create(20, 1, {});
	}

	void RendererAPI::SetViewportMousePosition(float x, float y) {
		m_ViewportMouseX = x;
		m_ViewportMouseY = y;
	}
}
