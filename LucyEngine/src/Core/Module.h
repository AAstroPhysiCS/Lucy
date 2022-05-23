#pragma once

#include "../Events/Event.h"
#include "Core/Metrics.h"

namespace Lucy {

	class Module {
	protected:
		Module() = default;
		~Module() = default;
	public:
		virtual void Init(RefLucy<Window> window) = 0;
		virtual void Begin(PerformanceMetrics& rendererMetrics) = 0;
		virtual void End() = 0;
		virtual void OnRender() = 0;
		virtual void OnEvent(Event& e) = 0;
		virtual void Destroy() = 0;
	protected:
		RefLucy<Window> m_Window;
	};

	extern Module* CreateEditorModule()
}