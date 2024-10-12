#pragma once

namespace Lucy {

	class RenderPipeline;
	class Scene;
	class Window;

	class Overlay {
	public:
		Overlay(const Ref<Scene>& scene) 
			: m_Scene(scene) {
		}
		virtual ~Overlay() = default;

		inline void SetRenderPipeline(Ref<RenderPipeline> renderPipeline) { m_RenderPipeline = renderPipeline; }

		virtual void OnRendererInit(const Ref<Window>& window) = 0;

		virtual void Begin() = 0;
		virtual void Render() = 0;
		virtual void End() = 0;

		virtual void OnEvent(Event& e) = 0;
		virtual void Destroy() = 0;
	protected:
		Ref<RenderPipeline> m_RenderPipeline = nullptr;
		Ref<Scene> m_Scene = nullptr;
	};
}