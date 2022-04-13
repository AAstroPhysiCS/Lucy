#include "lypch.h"
#include "RHI.h"

#include "Renderer/OpenGLRHI.h"
#include "Renderer/VulkanRHI.h"

namespace Lucy {

	RefLucy<RHI> RHI::Create(RenderArchitecture architecture) {
		switch (architecture) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLRHI>(architecture);
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanRHI>(architecture);
				break;
		}
		return nullptr;
	}
	
	RHI::RHI(RenderArchitecture renderArchitecture) {
		m_Architecture = renderArchitecture;
	}

	void RHI::SetViewportMousePosition(float x, float y) {
		m_ViewportMouseX = x;
		m_ViewportMouseY = y;
	}
}
