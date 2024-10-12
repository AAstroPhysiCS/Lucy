#pragma once

#include "Core/Base.h"
#include "Scene/Scene.h"

#include "Renderer/RenderGraph/RenderGraph.h"

namespace Lucy {

	class Image;

	enum class ViewMode : uint8_t {
		Lit,
		Wireframe,
	};

	struct RenderPipelineCreateInfo {
		ViewMode ViewMode = ViewMode::Wireframe;
		//TODO: Settings etc...
	};

	class RenderPipeline {
	public:
		RenderPipeline(const RenderPipelineCreateInfo& createInfo);
		virtual ~RenderPipeline() = default;

		virtual void BeginFrame() = 0;
		virtual void RenderFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void OnEvent(Event& e);
		
		virtual Ref<Image> GetOutputImage() = 0;

		void SetViewportMode(ViewMode mode) { m_CreateInfo.ViewMode = mode; }
		void SetViewportArea(int32_t width, int32_t height) { 
			m_ViewportWidth = width; 
			m_ViewportHeight = height; 
		}

		inline auto GetViewportArea() const {
			struct Size { int32_t Width, Height; };
			return Size{ m_ViewportWidth, m_ViewportHeight };
		}

		inline auto GetViewportMousePos() const {
			struct Size { float Width, Height; };
			return Size{ m_ViewportMouseX, m_ViewportMouseY };
		}
	private:
		RenderPipelineCreateInfo m_CreateInfo;

		int32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		float m_ViewportMouseX = 0, m_ViewportMouseY = 0;
	};
}