#pragma once

#include "Core/Window.h"
#include "Renderer/Device/RenderDevice.h"

#include "../RendererConfiguration.h"

namespace Lucy {

	class RenderContext : public MemoryTrackable {
	public:
		virtual ~RenderContext();

		inline const Ref<RenderDevice>& GetRenderDevice() const { return m_RenderDevice; }
		inline const Ref<Window>& GetWindow() const { return m_Window; }

		static Ref<RenderContext> Create(RenderArchitecture arch, const Ref<Window>& window);
	protected:
		RenderContext(const Ref<Window>& window, Ref<RenderDevice> renderDevice);
		
		virtual void PrintInfo() = 0;
		virtual void Destroy() = 0;
		virtual void Init() = 0;
	private:
		Ref<Window> m_Window = nullptr;
		Ref<RenderDevice> m_RenderDevice = nullptr;

		friend class RendererBackend; //for Destroy (safety)
	};
}

