#include "lypch.h"
#include "ViewportRenderPipeline.h"

#include "Renderer/Renderer.h"
#include "Renderer/RendererPasses.h"
#include "Renderer/Device/RenderDeviceScene.h"

#include "Utilities/Utilities.h"

#include "Scene/Components.h"
#include "Scene/Scene.h"

namespace Lucy {

	ViewportRenderPipeline::ViewportRenderPipeline(const RenderPipelineCreateInfo& createInfo, const Ref<Scene>& scene)
		: RenderPipeline(createInfo) {
		auto [viewportWidth, viewportHeight] = Utils::ReadAttributeFromIni("Viewport", "Size");
		SetViewportArea(viewportWidth, viewportHeight);

		Renderer::AddRendererPass<ForwardPBRPass>(scene, viewportWidth, viewportHeight);
		Renderer::AddRendererPass<CubemapPass>(scene, viewportWidth, viewportHeight);
		Renderer::AddRendererPass<IrradiancePass>(scene, CubemapPass::HDRImageSize);
		Renderer::AddRendererPass<BRDFLutPass>(512);
		Renderer::AddRendererPass<PrefilterPass>(scene, CubemapPass::HDRImageSize);
		Renderer::AddRendererPass<ShadowPass>(scene, 2048);
	}

	void ViewportRenderPipeline::BeginFrame(const Ref<RenderDevice>& device, Ref<Scene>& scene) {
		LUCY_PROFILE_NEW_EVENT("ViewportRenderPipeline::BeginFrame");

		auto& deviceScene = device->GetScene();

		scene->ViewForEach<DirectionalLightComponent>([&](DirectionalLightComponent& lightComponent) {
			RenderDeviceSceneGlobalData::LightValues lightValues{};

			lightValues.Direction = lightComponent.GetDirection();
			lightValues.Color = lightComponent.GetColor();

			ShadowCamera::ResetSplit();

			auto& shadowCameras = ShadowPass::GetShadowCameras();

			for (size_t i = 0; i < shadowCameras.size(); i++) {
				ShadowCamera& shadowCamera = shadowCameras[i];

				shadowCamera.SetRotation(lightValues.Direction);

				lightValues.DirLightShadowCascadeSplits[i] = shadowCamera.GetCascadeSplitDepth();

				const auto& vp = shadowCamera.GetCameraViewProjection();
				lightValues.DirLightShadowMatrices[i] = vp.Proj * vp.View;
			}

			deviceScene->UpdateLightValues(lightValues);
		});

		deviceScene->UpdateCamera(scene->GetEditorCamera().GetCameraViewProjection());
	}

	void ViewportRenderPipeline::RenderFrame() {
		LUCY_PROFILE_NEW_EVENT("ViewportRenderPipeline::RenderFrame");
		Renderer::ExecuteRenderGraph();
	}

	void ViewportRenderPipeline::EndFrame() {
		LUCY_PROFILE_NEW_EVENT("ViewportRenderPipeline::EndFrame");
	}

	Ref<Image> ViewportRenderPipeline::GetOutputImage() {
		return Renderer::GetFrameBufferOutputOfPass("PBRGeometryPass");
	}
}