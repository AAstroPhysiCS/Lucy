#pragma once

#include "Core/Module.h"

#include "Context/GraphicsPipeline.h"

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
		void OnViewportResize();

		inline const Ref<GraphicsPipeline>& GetGeometryPipeline() { return m_GeometryPipeline; }
		inline const Ref<GraphicsPipeline>& GetIDPipeline() { return m_IDPipeline; }
	private:
		Ref<GraphicsPipeline> m_GeometryPipeline = nullptr;
		Ref<GraphicsPipeline> m_IDPipeline = nullptr;
		Ref<GraphicsPipeline> m_CubemapPipeline = nullptr;
	};

	/* --- Individual Passes --- */
	extern void GeometryPass(void* commandBuffer, Ref<ContextPipeline> geometryPipeline, RenderCommand* staticMeshRenderCommand);
	extern void IDPass(void* commandBuffer, Ref<ContextPipeline> idPipeline, RenderCommand* staticMeshRenderCommand);
	extern void CubemapPass(void* commandBuffer, Ref<ContextPipeline> cubemapPipeline, RenderCommand* cubemapRenderCommand);
}