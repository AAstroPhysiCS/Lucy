#pragma once

#include "Core/Window.h"

#include "../RendererConfiguration.h"

namespace Lucy {

	class RenderThread;
	class RendererBackend;

	class RenderContext : public MemoryTrackable {
	public:
		static Ref<RenderContext> Create(RenderArchitecture arch, const Ref<Window>& window);
	public:
		virtual ~RenderContext() = default;

		virtual void Init() = 0;
		virtual void PrintInfo() = 0;

		inline const Ref<Window>& GetWindow() const { return m_Window; }
	protected:
		RenderContext(const Ref<Window>& window);
		
		virtual void Destroy() = 0;
	private:
		Ref<Window> m_Window = nullptr;
	};
}