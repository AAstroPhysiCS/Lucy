#pragma once

#include "Core/Window.h"

#include "../RenderArchitecture.h"

namespace Lucy {

	class RenderContext {
	public:
		virtual ~RenderContext() = default;

		static Ref<RenderContext> Create(RenderArchitecture arch, Ref<Window>& window);
	protected:
		RenderContext(Ref<Window>& window);
		
		virtual void PrintInfo() = 0;
		virtual void Destroy() = 0;
		virtual void Init() = 0;

		Ref<Window> m_Window = nullptr;

		friend class RendererBase; //for Destroy (safety)
	};
}

