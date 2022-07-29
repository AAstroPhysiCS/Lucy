#pragma once

#include "Core/Module.h"

namespace Lucy {

	class RendererModule : public Module {
	public:
		RendererModule(RenderArchitecture arch, Ref<Window> window, Ref<Scene> scene);
		virtual ~RendererModule() = default;

		void Begin() final override;
		void End() final override;
		void OnRender() final override;
		void OnEvent(Event& e) final override;
		void Destroy() final override;
		void Wait() final override;

		void OnWindowResize();

		inline const Ref<Pipeline>& GetGeometryPipeline() { return m_GeometryPipeline; }
		inline const Ref<Pipeline>& GetIDPipeline() { return m_IDPipeline; }
	private:
		Ref<Pipeline> m_GeometryPipeline = nullptr;
		Ref<Pipeline> m_IDPipeline = nullptr;
	};
}