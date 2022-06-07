#include "lypch.h"
#include "RHI.h"

#include "Renderer/OpenGLRHI.h"
#include "Renderer/VulkanRHI.h"

namespace Lucy {

	RefLucy<RHI> RHI::Create(RenderArchitecture arch) {
		switch (arch) {
			case RenderArchitecture::OpenGL:
				return CreateRef<OpenGLRHI>(arch);
				break;
			case RenderArchitecture::Vulkan:
				return CreateRef<VulkanRHI>(arch);
				break;
		}
		return nullptr;
	}
	
	RHI::RHI(RenderArchitecture renderArchitecture) {
		m_Architecture = renderArchitecture;
	}

	void RHI::ClearQueues() {
		m_StaticMeshDrawCommandQueue.clear();
		s_CommandQueue.Clear();
	}
}
