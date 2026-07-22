#pragma once

#include "Renderer/Memory/Memory.h"

#include "Renderer/RendererConfiguration.h"

namespace Lucy {

	class Semaphore;

	class Window;
	class RenderDevice;

	class SwapChain : public MemoryTrackable {
	public:
		SwapChain(const Ref<Window>& window, const Ref<RenderDevice>& renderDevice);
		virtual ~SwapChain() = default;

		static Ref<SwapChain> Create(RenderArchitecture arch, const Ref<Window>& window, const Ref<RenderDevice>& renderDevice);

		virtual void Init() = 0;
		virtual void Recreate() = 0;
		virtual RenderContextResultCodes AcquireNextImage(Semaphore* currentFrameImageAvailSemaphore, uint32_t& imageIndex) = 0;
		virtual RenderContextResultCodes Present(Semaphore* signalSemaphore, uint32_t& imageIndex) = 0;

		virtual void Destroy() = 0;

		inline const Ref<RenderDevice>& GetRenderDevice() const { return m_RenderDevice; }
		inline const Ref<Window>& GetWindow() const { return m_Window; }
	private:
		Ref<Window> m_Window = nullptr;
		Ref<RenderDevice> m_RenderDevice = nullptr;
	};
}