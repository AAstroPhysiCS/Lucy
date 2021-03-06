#pragma once

#include "Context/Pipeline.h"

#include "Memory/Buffer/FrameBuffer.h"
#include "RenderPass.h"

namespace Lucy {

	class Scene;

	struct ImGuiPipeline {
		Ref<RenderPass> UIRenderPass = nullptr;
		Ref<FrameBuffer> UIFramebuffer = nullptr;
	};

	class ViewportRenderer {
	public:
		ViewportRenderer() = default;
		~ViewportRenderer() = default;

		void Init();
		void Begin(Scene& scene);
		void Dispatch(Scene& scene);
		void End();
		void Destroy();

		static Ref<Pipeline> GetGeometryPipeline() { return s_GeometryPipeline; }
		static ImGuiPipeline GetImGuiPipeline() { return s_ImGuiPipeline; }

		void OnWindowResize();
	private:
		void GeometryPass();
		void IDPass();
		void UIPass();

		inline static Ref<Pipeline> s_GeometryPipeline = nullptr;
		inline static ImGuiPipeline s_ImGuiPipeline;

		friend class VulkanRHI;
		friend class OpenGLRHI;
	};
}

