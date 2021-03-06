#pragma once

#include "Core/Module.h"

#include "Renderer/ViewportRenderer.h"

namespace Lucy {

	class RendererModule : public Module {
	public:
		RendererModule(Ref<Window> window, Ref<Scene> scene);
		virtual ~RendererModule() = default;

		void Begin() override;
		void End() override;
		void OnRender() override;
		void OnEvent(Event& e) override;
		void Destroy() override;
	private:
		ViewportRenderer m_ViewportRenderer;
	};
}

