#pragma once

#include "Scene/Scene.h"
#include "Context/Pipeline.h"
#include "Buffer/FrameBuffer.h"
#include "RenderPass.h"

namespace Lucy {
	
	struct ImGuiPipeline {
		RefLucy<RenderPass> m_UIRenderPass = nullptr;
		RefLucy<FrameBuffer> m_UIFramebuffer = nullptr;
	};
	
	class ViewportRenderer {
	public:
		ViewportRenderer() = default;
		~ViewportRenderer() = default;

		void Init();
		void Begin(Scene& scene);
		void Dispatch();
		void End();
		void Destroy();

		void OnWindowResize();
	private:
		void UIPass();
		void GeometryPass();
		void IDPass();

		inline static RefLucy<Pipeline> s_GeometryPipeline = nullptr;
		inline static ImGuiPipeline s_ImGuiPipeline;
		inline static Scene* s_ActiveScene = nullptr;

		friend class ImGuiLayer; //for ImGuiPipeline
	};
}

