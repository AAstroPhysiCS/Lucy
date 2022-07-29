#include "lypch.h"
#include "RendererBase.h"

#include "Renderer/VulkanRenderer.h"
#include "Descriptors/DescriptorSetManager.h"

namespace Lucy {

	RendererBase::RendererBase(RenderArchitecture arch, Ref<Window>& window) {
		m_RenderContext = RenderContext::Create(arch, window);
		m_RenderDevice = RenderDevice::Create();
	}

	Ref<RendererBase> RendererBase::Create(RenderArchitecture arch, Ref<Window>& window) {
		switch (arch) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanRenderer>(arch, window);
				break;
		}
	}

	void RendererBase::Init() {
		m_RenderContext->PrintInfo();
		m_RenderDevice->Init();

		DescriptorSetManager::Init();
	}

	void RendererBase::Destroy() {
		m_RenderDevice->Destroy();
		m_RenderContext->Destroy();
	}
}