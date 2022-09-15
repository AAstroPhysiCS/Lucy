#include "lypch.h"
#include "RendererBase.h"

#include "Renderer/VulkanRenderer.h"
#include "Context/VulkanSwapChain.h"

namespace Lucy {

	RendererBase::RendererBase(RenderArchitecture arch, Ref<Window>& window) {
		m_RenderContext = RenderContext::Create(arch, window);
		m_RenderDevice = RenderDevice::Create();
	
		m_MaxFramesInFlight = VulkanSwapChain::Get().m_SwapChainImages.size();
	}

	Ref<RendererBase> RendererBase::Create(RenderArchitecture arch, Ref<Window>& window) {
		switch (arch) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanRenderer>(arch, window);
				break;
		}
		return nullptr;
	}

	void RendererBase::Init() {
		m_RenderContext->PrintInfo();
		m_RenderDevice->Init();
	}

	void RendererBase::Destroy() {
		m_RenderDevice->Destroy();
		m_RenderContext->Destroy();
	}
}