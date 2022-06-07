#pragma once

#include "Scene/Scene.h"
#include "Context/Pipeline.h"
#include "Buffer/FrameBuffer.h"
#include "RenderPass.h"
#include "CommandQueue.h"

namespace Lucy {
	
	struct ImGuiPipeline {
		RefLucy<RenderPass> UIRenderPass = nullptr;
		RefLucy<FrameBuffer> UIFramebuffer = nullptr;
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

		static RefLucy<Pipeline> GetGeometryPipeline() { return s_GeometryPipeline; }
		static ImGuiPipeline GetImGuiPipeline() { return s_ImGuiPipeline; }

		void OnWindowResize();
	private:
		void GeometryPass();
		void IDPass();
		void UIPass();

		inline static RefLucy<Pipeline> s_GeometryPipeline = nullptr;
		inline static ImGuiPipeline s_ImGuiPipeline;

		friend class VulkanRHI;
		friend class OpenGLRHI;
	};
}

