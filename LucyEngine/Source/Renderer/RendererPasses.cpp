#include "lypch.h"
#include "RendererPasses.h"

#include "Renderer.h"
#include "RenderGraph/RenderGraphBuilder.h"
#include "RenderGraph/RenderGraphRegistry.h"

#include "Memory/Buffer/Buffer.h"
#include "Memory/Buffer/PushConstant.h"

#include "Pipeline/ComputePipeline.h"

#include "Scene/Components.h"

namespace Lucy {

	/*
	TODO:
		Material should indicate which shader we gonna render on to
		Group meshes by material / shader. And loop it through.
		with that we can bind image handle to accordingly and with no performance drop

		we have to separate mesh and materials. materials should set each thing
	*/

#pragma region ForwardPBRPass

	ForwardPBRPass::ForwardPBRPass(Ref<Scene> scene, uint32_t width, uint32_t height)
		: m_Scene(scene), m_Width(width), m_Height(height) {
	}

	void ForwardPBRPass::AddPass(const Ref<RenderGraph>& renderGraph) {

		renderGraph->AddPass(TargetQueueFamily::Graphics, "PBRGeometryPass", [=, *this](RenderGraphBuilder& build) {
			build.SetViewportArea(m_Width, m_Height);
			build.SetInFlightMode(true);

			build.DeclareImage(RGResource(GeometryImage), {
				.Width = m_Width,
				.Height = m_Height,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorAttachment,
				.Format = ImageFormat::R8G8B8A8_UNORM,
				.GenerateSampler = true,
				.ImGuiUsage = true,
				}, RenderPassLoadStoreAttachments::ClearStore,
				RGResource(GeometryDepthImage), {
					.Width = m_Width,
					.Height = m_Height,
					.ImageType = ImageType::Type2D,
					.ImageUsage = ImageUsage::AsDepthAttachment,
					.Format = ImageFormat::D32_SFLOAT,
					.GenerateSampler = true,
				}, RenderPassLoadStoreAttachments::ClearStore
			);

			build.ReadImage(RGResource(ShadowImages), RenderGraphResourceAccess::ShaderSampledRead);
			build.ReadExternalImage(RGResource(BRDFLutImage), RenderGraphResourceAccess::ShaderSampledRead);
			//build.ReadImage(RGResource(PrefilterImage), RenderGraphResourceAccess::ShaderSampledRead);
			//build.ReadImage(RGResource(IrradianceImage), RenderGraphResourceAccess::ShaderSampledRead);

			build.BindRenderTarget(RGResource(GeometryImage), RGResource(GeometryDepthImage));

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("PBRGeometryPipeline");
				const auto& settings = Renderer::GetRendererSettings();

				m_Scene->ViewForEach<DirectionalLightComponent>([&](DirectionalLightComponent& lightComponent) {
					const auto& lightningAttributes = pipeline->GetUniformBufferIfExists("LightValues");
					lightningAttributes->SetData((uint8_t*)&lightComponent, sizeof(DirectionalLightComponent));

					glm::vec4 shadowCameraFarPlanes;

					ShadowCamera::ResetSplit();

					auto& shadowCameras = ShadowPass::GetShadowCameras();
					for (size_t i = 0; ShadowCamera& shadowCamera : shadowCameras) {
						shadowCamera.SetRotation(lightComponent.GetDirection());

						shadowCameraFarPlanes[i++] = shadowCamera.GetCascadeSplitDepth();

						const auto& vp = shadowCamera.GetCameraViewProjection();
						glm::mat4 shadowCameraMatrix = vp.Proj * vp.View;
						lightningAttributes->Append((uint8_t*)&shadowCameraMatrix, sizeof(glm::mat4));
					}

					lightningAttributes->Append((uint8_t*)&shadowCameraFarPlanes, sizeof(glm::vec4));
				});

				pipeline->BindImageHandleTo("u_ShadowMap", registry.GetImage(RGResource(ShadowImages)));
				pipeline->BindImageHandleTo("u_BRDFLut", registry.GetImage(RGResource(BRDFLutImage)));

				bool imageBound = false;

				m_Scene->ViewForEach<HDRCubemapComponent>([&pipeline, &registry, &imageBound, &settings](const HDRCubemapComponent& hdrComponent) {
					if (!hdrComponent.IsPrimary || imageBound)
						return;
#if USE_COMPUTE_FOR_CUBEMAP_GEN
					pipeline->BindImageHandleTo("u_IrradianceMap", hdrComponent.GetIrradianceImage());
#else
					pipeline->BindImageHandleTo("u_IrradianceMap", registry.GetImage(RGResource(IrradianceImage)));
#endif
					pipeline->BindImageHandleTo("u_PrefilterMap", registry.GetImage(RGResource(PrefilterImage)));
					imageBound = true;
				});

				if (!pipeline->HasImageHandleBoundTo("u_IrradianceMap"))
					pipeline->BindImageHandleTo("u_IrradianceMap", Renderer::GetBlankCubeImage());

				if (!pipeline->HasImageHandleBoundTo("u_PrefilterMap"))
					pipeline->BindImageHandleTo("u_PrefilterMap", Renderer::GetBlankCubeImage());

				if (auto cameraBuffer = pipeline->GetUniformBufferIfExists("Camera")) {
					auto vp = m_Scene->GetEditorCamera().GetCameraViewProjection();
					cameraBuffer->SetData((uint8_t*)&vp, sizeof(vp));
				}

				RenderCommand& draw = cmdList.BeginRenderCommand("PBRForwardPass");

				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();

				m_Scene->ViewRForEach<MeshComponent, TransformComponent>([&](MeshComponent& meshComponent, TransformComponent& transformComponent) {
					draw.DrawIndexedMeshWithMaterial(meshComponent.GetMesh(), transformComponent.GetMatrix());
				});

				cmdList.EndRenderCommand();
			};
		});

		renderGraph->AddPass(TargetQueueFamily::Graphics, "IDPass", [=, *this](RenderGraphBuilder& build) {
			build.SetViewportArea(m_Width, m_Height);
			build.SetInFlightMode(true);

			build.DeclareImage(RGResource(IDPassImage), {
				.Width = m_Width,
				.Height = m_Height,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorTransferAttachment,
				.Format = ImageFormat::R8G8B8A8_UNORM,
				.GenerateSampler = true
			}, RenderPassLoadStoreAttachments::ClearStore,
				RGResource(IDPassDepthImage), {
				.Width = m_Width,
				.Height = m_Height,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsDepthAttachment,
				.Format = ImageFormat::D32_SFLOAT,
				.GenerateSampler = true,
			}, RenderPassLoadStoreAttachments::ClearStore);

			build.BindRenderTarget(RGResource(IDPassImage), RGResource(IDPassDepthImage));

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("IDPipeline");

				if (auto cameraBuffer = pipeline->GetUniformBufferIfExists("Camera")) {
					auto vp = m_Scene->GetEditorCamera().GetCameraViewProjection();
					cameraBuffer->SetData((uint8_t*)&vp, sizeof(vp));
				}

				RenderCommand& draw = cmdList.BeginRenderCommand("IDPass");
				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();

				m_Scene->ViewRForEach<MeshComponent, TransformComponent>([&](MeshComponent& meshComponent, TransformComponent& transformComponent) {
					draw.DrawIndexedMeshWithMaterial(meshComponent.GetMesh(), transformComponent.GetMatrix());
				});

				cmdList.EndRenderCommand();
			};
		});
	}

#pragma endregion ForwardPBRPass

#pragma region ShadowPass

	ShadowPass::ShadowPass(Ref<Scene> scene, uint32_t size)
		: m_Scene(scene), m_ShadowMapSize(size) {
	}

	void ShadowPass::AddPass(const Ref<RenderGraph>& renderGraph) {

		renderGraph->AddPass(TargetQueueFamily::Graphics, "VSMPass", [=, *this](RenderGraphBuilder& build) {
			build.SetViewportArea(m_ShadowMapSize, m_ShadowMapSize);
			build.SetClearColor({ 1.0f, 1.0f, 1.0f, 1.0f });

			build.DeclareImage(RGResource(ShadowImages), {
				.Width = m_ShadowMapSize,
				.Height = m_ShadowMapSize,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
				.Layers = ShadowPass::NUM_CASCADES,
				.Format = ImageFormat::R32G32_SFLOAT,
				.GenerateSampler = true,
			}, RenderPassLoadStoreAttachments::ClearStore,
				RGResource(VSMDepth), {
				.Width = m_ShadowMapSize,
				.Height = m_ShadowMapSize,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsDepthAttachment,
				.Layers = ShadowPass::NUM_CASCADES,
				.Format = ImageFormat::D32_SFLOAT,
				.GenerateSampler = true,
			}, RenderPassLoadStoreAttachments::ClearStore);

			build.BindRenderTarget(RGResource(ShadowImages), RGResource(VSMDepth));

			const auto& editorCamera = m_Scene->GetEditorCamera();
			InitializeShadowCameras(m_ShadowMapSize, editorCamera);

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("VSMPipeline");

				if (auto cameraBuffer = pipeline->GetUniformBufferIfExists("ShadowCameraVPs")) {
					glm::mat4 projMatrix[NUM_CASCADES];

					ShadowCamera::ResetSplit();
					for (size_t i = 0; ShadowCamera& shadowCamera : s_ShadowCameras) {
						shadowCamera.Update();

						const auto& vp = shadowCamera.GetCameraViewProjection();
						projMatrix[i] = vp.Proj * vp.View;
						i++;
					}

					cameraBuffer->SetData((uint8_t*)&projMatrix, sizeof(projMatrix));
				}

				RenderCommand& draw = cmdList.BeginRenderCommand("VSM Draw");
				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();

				m_Scene->ViewRForEach<MeshComponent, TransformComponent>([&](MeshComponent& meshComponent, TransformComponent& transformComponent) {
					draw.DrawIndexedMesh(meshComponent.GetMesh(), transformComponent.GetMatrix());
				});

				cmdList.EndRenderCommand();
			};
		});

		enum class GaussianBlurDirection : uint8_t {
			Horizontal,
			Vertical
		};

		const auto ExecuteGaussianBlur = [=](RenderGraphRegistry& registry, RenderCommandList& cmdList, GaussianBlurDirection direction) {
			const auto& shadowImages = registry.GetImage(RGResource(ShadowImages));
			const auto& shadowImagesBlurred = registry.GetImage(RGResource(ShadowImagesBlurred));

			auto width = shadowImages->GetWidth();
			auto height = shadowImages->GetHeight();

			const auto& blurPipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>(direction == GaussianBlurDirection::Horizontal 
				? "VSMHorizontalBlurComputePipeline" : "VSMVerticalBlurComputePipeline");
			auto& pushConstant = blurPipeline->GetPipelineConstants("PushConstants");

			RenderCommand& cmd = cmdList.BeginRenderCommand(direction == GaussianBlurDirection::Horizontal ? "VSMHorizontalBlur" : "VSMVerticalBlur");
			cmd.BindPipeline(blurPipeline);

			if (direction == GaussianBlurDirection::Horizontal) {
				int32_t dir[4] = { 1, 0, width, height };
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&dir), sizeof(dir));

				blurPipeline->BindImageHandleTo("u_InputMoments", shadowImages);
				blurPipeline->BindImageHandleTo("u_OutputMoments", shadowImagesBlurred);
			} else {
				int32_t dir[4] = { 0, 1, width, height };
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&dir), sizeof(dir));

				blurPipeline->BindImageHandleTo("u_InputMoments", shadowImagesBlurred);
				blurPipeline->BindImageHandleTo("u_OutputMoments", shadowImages);
			}

			cmd.UpdateDescriptorSets();
			cmd.BindAllDescriptorSets();
			cmd.BindPushConstant(pushConstant);
			cmd.DispatchCompute((width + 7) / 8, (height + 7) / 8, NUM_CASCADES);

			cmdList.EndRenderCommand();
		};

		renderGraph->AddPass(TargetQueueFamily::Compute, "VSMHorizontalBlurCompute", [=, *this](RenderGraphBuilder& build) {
			build.DeclareImage(RGResource(ShadowImagesBlurred), {
				.Width = m_ShadowMapSize,
				.Height = m_ShadowMapSize,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
				.Layers = ShadowPass::NUM_CASCADES,
				.Format = ImageFormat::R32G32_SFLOAT,
				.GenerateSampler = true,
			}, RenderPassLoadStoreAttachments::ClearDontCare);

			build.ReadImage(RGResource(ShadowImages), RenderGraphResourceAccess::StorageRead);
			build.WriteImage(RGResource(ShadowImagesBlurred), RenderGraphResourceAccess::StorageWrite);

			return std::bind(
				ExecuteGaussianBlur,
				std::placeholders::_1,
				std::placeholders::_2,
				GaussianBlurDirection::Horizontal
			);
		});

		renderGraph->AddPass(TargetQueueFamily::Compute, "VSMVerticalBlurCompute", [=, *this](RenderGraphBuilder& build) {
			build.ReadImage(RGResource(ShadowImagesBlurred), RenderGraphResourceAccess::StorageRead);
			build.WriteImage(RGResource(ShadowImages), RenderGraphResourceAccess::StorageWrite);

			return std::bind(
				ExecuteGaussianBlur,
				std::placeholders::_1,
				std::placeholders::_2,
				GaussianBlurDirection::Vertical
			);
		});
	}

	// The method is explained well here
	// https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-parallel-split-shadow-maps-programmable-gpus
	void ShadowPass::InitializeShadowCameras(uint32_t size, const EditorCamera& editorCamera) const {
		static constexpr float lambda = 0.95f;
		static float cascadeSplits[NUM_CASCADES];

		float n = editorCamera.GetNearPlane() * ShadowCamera::GetNearPlaneFactor();
		float f = editorCamera.GetFarPlane() * ShadowCamera::GetFarPlaneFactor();
		float clipRange = f - n;

		float minZ = n;
		float maxZ = n + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		for (uint32_t i = 0; i < NUM_CASCADES; i++) {
			float iDivM = (i + 1) / (float)NUM_CASCADES;
			float C_iLog = minZ * std::pow(ratio, iDivM);
			float C_iUniform = minZ + range * iDivM;
			float C_i = lambda * (C_iLog - C_iUniform) + C_iUniform;
			cascadeSplits[i] = (C_i - n) / clipRange;
		}

		for (uint32_t i = 0; i < NUM_CASCADES; i++)
			s_ShadowCameras.emplace_back(size, editorCamera, cascadeSplits[i]);
	}

	ShadowCamera::ShadowCamera(uint32_t size, const EditorCamera& editorCamera, float nearPlane, float farPlane)
		: OrthographicCamera(0.0f, 0.0f, 0.0f, 0.0f, nearPlane, farPlane), m_ShadowMapSize(size), m_EditorCamera(editorCamera) {
		UpdateView();

		float n = m_EditorCamera.GetNearPlane() * s_NearPlaneFactor;
		float f = m_EditorCamera.GetFarPlane() * s_FarPlaneFactor;
		float clipRange = f - n;

		m_CascadeSplitDepth = n + m_CascadeSplit * clipRange;
	}

	ShadowCamera::ShadowCamera(uint32_t size, const EditorCamera& editorCamera, float cascadeSplit)
		: OrthographicCamera(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f), m_ShadowMapSize(size), m_EditorCamera(editorCamera), m_CascadeSplit(cascadeSplit) {
		UpdateView();

		float n = m_EditorCamera.GetNearPlane() * s_NearPlaneFactor;
		float f = m_EditorCamera.GetFarPlane() * s_FarPlaneFactor;
		float clipRange = f - n;

		m_CascadeSplitDepth = n + m_CascadeSplit * clipRange;
	}

	void ShadowCamera::UpdateView() {
		CameraViewProjection editorVp = m_EditorCamera.GetCameraViewProjection();
		glm::mat4 invVP = glm::inverse(editorVp.Proj * editorVp.View);

		static constexpr uint32_t frustumCornerCount = 8;
		static constexpr glm::vec3 ndcCoordinates[frustumCornerCount] = {
			glm::vec3(-1.0f,  1.0f, 0.0f),	//near left bottom
			glm::vec3(1.0f,  1.0f, 0.0f),	//near right bottom
			glm::vec3(1.0f, -1.0f, 0.0f),	//near right top
			glm::vec3(-1.0f, -1.0f, 0.0f),	//near left top
			glm::vec3(-1.0f,  1.0f,  1.0f),	//far left bottom
			glm::vec3(1.0f,  1.0f,  1.0f),	//far right bottom
			glm::vec3(1.0f, -1.0f,  1.0f),	//far right top
			glm::vec3(-1.0f, -1.0f,  1.0f),	//far left top
		};

		glm::vec3 frustumCornersWS[frustumCornerCount];
		for (uint32_t i = 0; i < frustumCornerCount; i++) {
			glm::vec4 invCorner = invVP * glm::vec4(ndcCoordinates[i], 1.0f);
			frustumCornersWS[i] = invCorner / invCorner.w;
		}

		for (uint32_t i = 0; i < frustumCornerCount / 2; i++) {
			glm::vec3 cornerRay = frustumCornersWS[i + 4] - frustumCornersWS[i];
			glm::vec3 nearCornerRay = cornerRay * s_LastSplitDist;
			glm::vec3 farCornerRay = cornerRay * m_CascadeSplit;

			frustumCornersWS[i + 4] = frustumCornersWS[i] + farCornerRay;
			frustumCornersWS[i] = frustumCornersWS[i] + nearCornerRay;
		}

		glm::vec3 frustumCenter = glm::vec3(0.0f);
		for (uint32_t i = 0; i < frustumCornerCount; i++)
			frustumCenter += frustumCornersWS[i];
		frustumCenter /= frustumCornerCount;

		// Calculating the frustum based on this method, which incorpartes a circle to approximate the bounds of each frustum.
		// https://johanmedestrom.wordpress.com/2016/03/18/opengl-cascaded-shadow-maps/
		float radius = 0.0f;
		for (uint32_t i = 0; i < frustumCornerCount; i++) {
			float distance = glm::length(frustumCornersWS[i] - frustumCenter);
			radius = glm::max(radius, distance);
		}
		radius = std::ceil(radius * 16.0f) / 16.0f;
		
		glm::vec3 maxExtents = glm::vec3(radius, radius, radius);
		glm::vec3 minExtents = -maxExtents;

		const auto& lightDir = GetRotation();

		float radiusDistWS = std::ceil((frustumCornersWS[0] - frustumCornersWS[6]).length() * 16.0f) / 2.0f;

		float texelsPerUnitWS = m_ShadowMapSize / (radiusDistWS * 2.0f);

		glm::mat4 scalarMat = glm::mat4(1.0f);
		scalarMat = glm::scale(scalarMat, glm::vec3(texelsPerUnitWS));

		glm::mat4 lightLookAt = glm::lookAt(glm::vec3(0.0f), lightDir, s_UpDir);
		glm::mat4 scaledLightLookAt = scalarMat * lightLookAt;

		glm::vec4 scaledCenter = scaledLightLookAt * glm::vec4(frustumCenter, 1.0f);
		scaledCenter.x = std::floor(scaledCenter.x);
		scaledCenter.y = std::floor(scaledCenter.y);
		glm::vec3 snappedCenter = glm::inverse(scaledLightLookAt) * scaledCenter;

		glm::vec3 eye = snappedCenter - (lightDir * radiusDistWS * 2.0f);

		m_ViewMatrix = glm::mat4(1.0f);
		m_ViewMatrix = glm::lookAt(eye, snappedCenter, s_UpDir);

		m_Left = minExtents.x;
		m_Right = maxExtents.x;
		m_Bottom = minExtents.y;
		m_Top = maxExtents.y;
		m_NearPlane = minExtents.z * 6.0f;
		m_FarPlane = maxExtents.z * 6.0f;

		s_LastSplitDist = m_CascadeSplit;
	}

	void ShadowCamera::ResetSplit() {
		ShadowCamera::s_LastSplitDist = 0.0f;
	}

#pragma endregion ShadowPass

#pragma region CubemapPass

	CubemapPass::CubemapPass(Ref<Scene> scene, uint32_t width, uint32_t height)
		: m_Scene(scene), m_Width(width), m_Height(height) {
	}

	void CubemapPass::AddPass(const Ref<RenderGraph>& renderGraph) {

		renderGraph->AddPass(TargetQueueFamily::Graphics, "CubemapPass", [*this](RenderGraphBuilder& build) {
			build.SetViewportArea(m_Width, m_Height);
			build.SetInFlightMode(true);

			build.ReadImage(RGResource(GeometryImage), RenderGraphResourceAccess::ColorAttachmentWrite);
			build.ReadImage(RGResource(GeometryDepthImage), RenderGraphResourceAccess::DepthAttachmentWrite);
			build.BindRenderTarget(RGResource(GeometryImage), RGResource(GeometryDepthImage));

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("SkyboxPipeline");
				const auto& settings = Renderer::GetRendererSettings();

				bool imageBound = false;

				m_Scene->ViewForEach<HDRCubemapComponent>([&pipeline, &imageBound, &registry](const HDRCubemapComponent& hdrComponent) {
					if (!hdrComponent.IsPrimary || imageBound)
						return;
					pipeline->BindImageHandleTo("u_EnvironmentMap", registry.GetImage(RGResource(PrefilterImage)));
					imageBound = true;
				});

				if (!pipeline->HasImageHandleBoundTo("u_EnvironmentMap"))
					return;

				if (auto cameraBuffer = pipeline->GetUniformBufferIfExists("Camera")) {
					const auto& vp = m_Scene->GetEditorCamera().GetCameraViewProjection();
					cameraBuffer->SetData((uint8_t*)&vp, sizeof(vp));
				}

				if (auto paramBuffer = pipeline->GetUniformBufferIfExists("Params")) {
					paramBuffer->SetData((uint8_t*)&settings.EnvironmentLOD, sizeof(float));
				}

				const Ref<Mesh>& cubeMesh = Renderer::GetEnvCubeMesh();
				RenderCommand& draw = cmdList.BeginRenderCommand("Skybox Draw");

				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.DrawMesh(cubeMesh);

				cmdList.EndRenderCommand();
			};
		});

		renderGraph->AddPass(TargetQueueFamily::Graphics, "HDRImageToLayeredImage", [*this](RenderGraphBuilder& build) {
			build.SetViewportArea(HDRImageSize, HDRImageSize);

			build.ReadExternalTransientImage(RGResource(OriginalHDRImage), RenderGraphResourceAccess::ShaderSampledRead);

			build.DeclareImage(RGResource(HDRLayeredImage), {
				.Width = HDRImageSize,
				.Height = HDRImageSize,
				.ImageType = ImageType::TypeCube,
				.ImageUsage = ImageUsage::AsColorTransferAttachment,
				.Format = ImageFormat::R32G32B32A32_SFLOAT,
				.GenerateSampler = true,
			}, RenderPassLoadStoreAttachments::DontCareStore);

			build.BindRenderTarget(RGResource(HDRLayeredImage));

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("HDRImageToLayeredImageConvertPipeline");

				pipeline->BindImageHandleTo("u_EquirectangularMap", registry.GetImage(RGResource(OriginalHDRImage)));

				const auto& cubeMesh = Renderer::GetEnvCubeMesh();
				const uint32_t cubeMeshIndexCount = Renderer::GetEnvCubeMeshIndexCount();

				RenderCommand& draw = cmdList.BeginRenderCommand("Cubemap Prep Draw");
				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.BindBuffers(cubeMesh);

				static const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");

				ByteBuffer pushConstantData;
				pushConstantData.SetData((uint8_t*)&captureProjection, sizeof(captureProjection));
				pushConstant.SetData(pushConstantData);

				for (uint32_t i = 0; i < 6; i++) {
					draw.BindPushConstant(pushConstant);
					draw.DrawIndexed(cubeMeshIndexCount, 1, 0, 0, 0);
				}

				cmdList.EndRenderCommand();
			};
		});
	}

#pragma endregion CubemapPass

#pragma region IrradiancePass

	IrradiancePass::IrradiancePass(Ref<Scene> scene, uint32_t size)
		: m_Scene(scene), m_Size(size) {
	}

	void IrradiancePass::AddPass(const Ref<RenderGraph>&renderGraph) {
#if USE_COMPUTE_FOR_CUBEMAP_GEN
		renderGraph->AddPass(TargetQueueFamily::Compute, "IrradiancePass", [*this](RenderGraphBuilder& build) {
			build.ReadImage(RGResource(HDRLayeredImage), RenderGraphResourceAccess::StorageRead);

			build.ReadExternalImage(RGResource(IrradianceImage), RenderGraphResourceAccess::StorageRead);
			build.WriteImage(RGResource(IrradianceImage), RenderGraphResourceAccess::StorageWrite);

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				static constexpr const uint32_t layerCount = 6;
				static constexpr const uint32_t workGroupSize = 8;

				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("IrradianceComputePipeline");

				const auto& cubeImage = registry.GetImage(RGResource(HDRLayeredImage));
				const auto& irradianceImage = registry.GetImage(RGResource(IrradianceImage));

				RenderCommand& draw = cmdList.BeginRenderCommand("Irradiance Draw Compute");

				//draw.SetImageLayout(cubeImage, VK_IMAGE_LAYOUT_GENERAL, 0, 0, 1, layerCount);
				//draw.SetImageLayout(irradianceImage, VK_IMAGE_LAYOUT_GENERAL, 0, 0, 1, layerCount);

				pipeline->BindImageHandleTo("u_EnvironmentMapCompute", cubeImage);
				pipeline->BindImageHandleTo("u_EnvironmentIrradianceMapCompute", irradianceImage);

				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.DispatchCompute(m_Size / workGroupSize, m_Size / workGroupSize, layerCount);

				//draw.SetImageLayout(cubeImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, 1, layerCount);
				//draw.SetImageLayout(irradianceImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0, 1, layerCount);

				cmdList.EndRenderCommand();
			};
		});
#else
		renderGraph->AddPass(TargetQueueFamily::Graphics, "IrradiancePass", [*this](RenderGraphBuilder& build) {
			build.SetViewportArea(m_Size, m_Size);

			build.ReadImage(RGResource(HDRLayeredImage));

			build.DeclareImage(RGResource(IrradianceImage), {
				.Width = m_Size,
				.Height = m_Size,
				.ImageType = ImageType::TypeCube,
				.ImageUsage = ImageUsage::AsColorAttachment,
				.Format = ImageFormat::R16G16B16A16_SFLOAT,
				.GenerateSampler = true,
				.GenerateMipmap = MipmapCreateInfo::NoMipmap(),
				.ImGuiUsage = false,
			}, RenderPassLoadStoreAttachments::ClearDontCare);

			build.BindRenderTarget(RGResource(IrradianceImage));

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("IrradiancePipeline");
				const auto& mesh = Renderer::GetEnvCubeMesh();

				RenderCommand& draw = cmdList.BeginRenderCommand("Irradiance Draw");
				const auto& environmentMap = registry.GetImage(RGResource(HDRLayeredImage));

				pipeline->BindImageHandleTo("u_EnvironmentMap", environmentMap);

				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.BindBuffers(mesh);
				draw.DrawIndexed(36, 1, 0, 0, 0);

				cmdList.EndRenderCommand();
			};
		});
#endif
	}
#pragma endregion IrradiancePass

#pragma region PrefilterPass

	PrefilterPass::PrefilterPass(Ref<Scene> scene, uint32_t cubemapSize)
		: m_Scene(scene), m_CubemapSize(cubemapSize) {
	}

	void PrefilterPass::AddPass(const Ref<RenderGraph>& renderGraph) {
		renderGraph->AddPass(TargetQueueFamily::Compute, "PrefilterPass", [*this](RenderGraphBuilder& build) {
			build.DeclareImage(RGResource(PrefilterImage), {
				.Width = m_CubemapSize,
				.Height = m_CubemapSize,
				.ImageType = ImageType::TypeCube,
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
				.Format = ImageFormat::R32G32B32A32_SFLOAT,
				.GenerateSampler = true,
				.GenerateMipmap = MipmapCreateInfo::FromLevel(MAX_MIP_LEVELS, true),
			}, RenderPassLoadStoreAttachments::ClearDontCare);

			build.ReadImage(RGResource(HDRLayeredImage), RenderGraphResourceAccess::StorageRead);
			build.WriteImage(RGResource(PrefilterImage), RenderGraphResourceAccess::StorageWrite);

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("PrefilterComputePipeline");

				static constexpr const uint32_t workGroupSize = 8;
				
				pipeline->BindImageHandleTo("u_EnvironmentMap", registry.GetImage(RGResource(HDRLayeredImage)));

				RenderCommand& cmd = cmdList.BeginRenderCommand("Prefilter Draw Compute");
				cmd.BindPipeline(pipeline);

				for (size_t mip = 0; mip < MAX_MIP_LEVELS; mip++) {
					pipeline->BindImageHandleTo("u_EnvironmentPrefilterMapOut", registry.GetImage(RGResource(PrefilterImage)), mip);
				}
				cmd.UpdateDescriptorSets();
				cmd.BindAllDescriptorSets();

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				for (uint32_t mip = 0; mip < MAX_MIP_LEVELS; mip++) {
					const uint32_t mipSize = std::max(1u, m_CubemapSize >> mip);
					glm::vec4 prefilterParams = glm::vec4(mipSize, mipSize, mip / float(MAX_MIP_LEVELS - 1), mip);
					pushConstant.SetData((uint8_t*)&prefilterParams, sizeof(glm::vec4));

					const uint32_t groupsX = (mipSize + workGroupSize - 1) / workGroupSize;
					const uint32_t groupsY = (mipSize + workGroupSize - 1) / workGroupSize;

					cmd.BindPushConstant(pushConstant);
					cmd.DispatchCompute(groupsX, groupsY, 6);
				}

				cmdList.EndRenderCommand();
			};
		});
	}
#pragma endregion PrefilterPass

#pragma region BRDFLutPass

	BRDFLutPass::BRDFLutPass(uint32_t size) 
		: m_Size(size) {
	}

	void BRDFLutPass::AddPass(const Ref<RenderGraph>& renderGraph) {
		renderGraph->AddPass(TargetQueueFamily::Compute, "BRDFLutPass", [*this](RenderGraphBuilder& build) {
			build.SetExecutionPolicy(RenderGraphExecutionPolicy::Once);

			build.ReadExternalImage(RGResource(BRDFLutImage), RenderGraphResourceAccess::StorageRead);
			build.WriteExternalImage(RGResource(BRDFLutImage), RenderGraphResourceAccess::StorageWrite);

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				static constexpr const uint32_t workGroupSize = 8;

				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("BRDFLutComputePipeline");
				const auto& brdfLutImage = registry.GetImage(RGResource(BRDFLutImage));

				RenderCommand& draw = cmdList.BeginRenderCommand("BRDFLut Draw Compute");

				pipeline->BindImageHandleTo("u_BRDFLut", brdfLutImage);

				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.DispatchCompute(m_Size / workGroupSize, m_Size / workGroupSize, 1);

				cmdList.EndRenderCommand();
			};
		});

		Renderer::EnqueueToRenderCommandQueue([width = m_Size, height = m_Size](const Ref<RenderDevice>& device) {
			auto imageHandle = device->CreateImage({
				.Width = width,
				.Height = height,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
				.Format = ImageFormat::R16G16_SFLOAT,
				.GenerateSampler = true,
			}, "BRDFLutImage");

			Renderer::ImportExternalRenderGraphResource(RGResource(BRDFLutImage), imageHandle);
		});
	}
#pragma endregion BRDFLutPass
}