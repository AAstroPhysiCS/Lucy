#include "lypch.h"
#include "ViewportRenderPipeline.h"

#include "Renderer/Renderer.h"
#include "Renderer/RendererPasses.h"

#include "Renderer/RenderGraph/RenderGraph.h"

#include "Events/EventHandler.h"

#include "Scene/Entity.h"

#include "Utilities/Utilities.h"

namespace Lucy {

	ViewportRenderPipeline::ViewportRenderPipeline(const RenderPipelineCreateInfo& createInfo, const Ref<Scene>& scene)
		: RenderPipeline(createInfo) {
		auto [viewportWidth, viewportHeight] = Utils::ReadAttributeFromIni("Viewport", "Size");
		SetViewportArea(viewportWidth, viewportHeight);

		Renderer::AddRendererPass<ForwardPBRPass>(scene, viewportWidth, viewportHeight);
		Renderer::AddRendererPass<CubemapPass>(scene, viewportWidth, viewportHeight);
		Renderer::AddRendererPass<ShadowPass>(scene, 2048);
	}

	void ViewportRenderPipeline::BeginFrame() {
		LUCY_PROFILE_NEW_EVENT("ViewportRenderPipeline::BeginFrame");
	}

	void ViewportRenderPipeline::RenderFrame() {
		LUCY_PROFILE_NEW_EVENT("ViewportRenderPipeline::RenderFrame");
		Renderer::ExecuteRenderGraph();
	}

	void ViewportRenderPipeline::EndFrame() {
		LUCY_PROFILE_NEW_EVENT("ViewportRenderPipeline::EndFrame");
	}

	Ref<Image> ViewportRenderPipeline::GetOutputImage() {
		return Renderer::GetOutputOfPass("PBRGeometryPass");
	}
}