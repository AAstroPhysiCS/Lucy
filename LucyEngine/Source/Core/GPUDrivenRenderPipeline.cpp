#include "lypch.h"
#include "GPUDrivenRenderPipeline.h"

#include "Renderer/Renderer.h"
#include "Renderer/RendererPasses.h"
#include "Renderer/Device/RenderDeviceScene.h"

#include "Utilities/Utilities.h"

#include "Scene/Components.h"
#include "Scene/Scene.h"

#include <glm/gtc/matrix_access.hpp>

namespace Lucy {

	GPUDrivenRenderPipeline::GPUDrivenRenderPipeline(const RenderPipelineCreateInfo& createInfo, const Ref<Scene>& scene)
		: RenderPipeline(createInfo) {
		auto [viewportWidth, viewportHeight] = Utils::ReadAttributeFromIni("Viewport", "Size");
		SetViewportArea(viewportWidth, viewportHeight);

		auto& deviceScene = createInfo.RenderDevice->GetScene();
		m_MainCullViewHandle = deviceScene->RegisterCullView({});

		Renderer::AddRendererPass<GPUDrivenRendererPass>(createInfo.RenderDevice);
		Renderer::AddRendererPass<ForwardPBRPass>(scene, viewportWidth, viewportHeight);
		Renderer::AddRendererPass<CubemapPass>(scene, viewportWidth, viewportHeight);
		Renderer::AddRendererPass<IrradiancePass>(scene, CubemapPass::HDRImageSize);
		Renderer::AddRendererPass<BRDFLutPass>(512);
		Renderer::AddRendererPass<PrefilterPass>(scene, CubemapPass::HDRImageSize);
		Renderer::AddRendererPass<ShadowPass>(createInfo.RenderDevice, scene, 2048);
	}

	void GPUDrivenRenderPipeline::BeginFrame(const Ref<RenderDevice>& device, Ref<Scene>& scene) {
		LUCY_PROFILE_NEW_EVENT("GPUDrivenRenderPipeline::BeginFrame");
		auto& settings = Renderer::GetRendererSettings();

		auto& deviceScene = device->GetScene();
		const auto& camera = scene->GetEditorCamera();

		const auto ExtractFrustumPlanes = [](const glm::mat4& viewProjection, glm::vec4(&frustumPlanes)[6]) {
			glm::vec4 row0 = glm::row(viewProjection, 0);
			glm::vec4 row1 = glm::row(viewProjection, 1);
			glm::vec4 row2 = glm::row(viewProjection, 2);
			glm::vec4 row3 = glm::row(viewProjection, 3);

			frustumPlanes[0] = row3 + row0;
			frustumPlanes[1] = row3 - row0;
			frustumPlanes[2] = row3 + row1;
			frustumPlanes[3] = row3 - row1;
			frustumPlanes[4] = row2;
			frustumPlanes[5] = row3 - row2;

			for (glm::vec4& plane : frustumPlanes) {
				float length = glm::length(glm::vec3{ plane });
				if (length > 0.0f)
					plane /= length;
			}
		};

		scene->ViewForEach<DirectionalLightComponent>([&](DirectionalLightComponent& lightComponent) {
			RenderDeviceSceneGlobalData::LightValues lightValues{};
			lightValues.Direction = lightComponent.GetDirection();
			lightValues.Color = lightComponent.GetColor();

			ShadowCamera::ResetSplit();

			auto& shadowCameras = ShadowPass::GetShadowCameras();
			for (size_t i = 0; i < shadowCameras.size(); i++) {
				ShadowCamera& shadowCamera = shadowCameras[i];
				auto shadowMapSize = shadowCamera.GetShadowMapSize();

				shadowCamera.SetRotation(lightValues.Direction);

				const auto& vp = shadowCamera.GetCameraViewProjection();

				RenderDeviceCullViewData view{};
				view.View = vp.View;
				view.Projection = vp.Proj;
				view.ViewProjection = vp.Proj * vp.View;
				view.Viewport = { shadowMapSize, shadowMapSize, 1.0f / shadowMapSize, 1.0f / shadowMapSize };
				view.Data.w = static_cast<uint32_t>(GPUCullViewFlags::EnableFrustumCulling) | static_cast<uint32_t>(GPUCullViewFlags::Orthographic);

				ExtractFrustumPlanes(view.ViewProjection, view.FrustumPlanes);

				deviceScene->UpdateCullView(shadowCamera.GetCullViewHandle(), view);

				lightValues.DirLightShadowCascadeSplits[i] = shadowCamera.GetCascadeSplitDepth();
				lightValues.DirLightShadowMatrices[i] = view.ViewProjection;
			}

			deviceScene->UpdateLightValues(lightValues);
		});

		const auto& cameraViewProj = camera.GetCameraViewProjection();
		deviceScene->UpdateCamera(cameraViewProj);

		scene->ViewRForEach<MeshComponent, TransformComponent>([&](MeshComponent& meshComponent, TransformComponent& transformComponent) {
			const auto& handle = meshComponent.GetRenderDeviceObjectHandle();
			if (!handle)
				return;
			deviceScene->UpdateObjectTransform(handle, transformComponent.GetMatrix());
		});

		const auto [width, height] = GetViewportArea();

		RenderDeviceCullViewData cullView{};
		cullView.View = cameraViewProj.View;
		cullView.Projection = cameraViewProj.Proj;
		cullView.ViewProjection = cameraViewProj.Proj * cameraViewProj.View;

		const glm::mat4 inverseView = glm::inverse(cameraViewProj.View);
		cullView.CameraPosition = glm::vec4{ camera.GetPosition(), 1.0f };
		cullView.Viewport = { static_cast<float>(width), static_cast<float>(height), 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) };

		ExtractFrustumPlanes(cullView.ViewProjection, cullView.FrustumPlanes);

		uint32_t cullViewFlags = settings.CullViewFlags;

		cullView.Data.x = INVALID_INDEX;
		cullView.Data.y = INVALID_INDEX;
		cullView.Data.z = 0;
		cullView.Data.w = cullViewFlags;

		if ((cullViewFlags & static_cast<uint32_t>(GPUCullViewFlags::CameraFreeze)) == 0)
			deviceScene->UpdateCullView(m_MainCullViewHandle, cullView);
	}

	void GPUDrivenRenderPipeline::RenderFrame() {
		LUCY_PROFILE_NEW_EVENT("GPUDrivenRenderPipeline::RenderFrame");
		Renderer::ExecuteRenderGraph();
	}

	void GPUDrivenRenderPipeline::EndFrame() {
		LUCY_PROFILE_NEW_EVENT("GPUDrivenRenderPipeline::EndFrame");
	}

	Ref<Image> GPUDrivenRenderPipeline::GetOutputImage() {
		return Renderer::GetFrameBufferOutputOfPass("PBRGeometryPass");
	}
}