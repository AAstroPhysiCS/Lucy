#include "lypch.h"
#include "RendererPasses.h"

#include "Renderer.h"
#include "RenderGraph/RenderGraphBuilder.h"
#include "RenderGraph/RenderGraphRegistry.h"

#include "Memory/Buffer/Buffer.h"
#include "Memory/Buffer/PushConstant.h"

#include "Pipeline/ComputePipeline.h"

#include "Scene/Components.h"

#include "glm/gtx/euler_angles.hpp"

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
			}, RenderPassLoadStoreAttachments::ClearStore);

			build.ReadImage(RGResource(ShadowImages));

			build.BindRenderTarget(RGResource(GeometryImage), RGResource(GeometryDepthImage));

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("PBRGeometryPipeline");
				const auto& shader = pipeline->GetShader();

				m_Scene->ViewForEach<DirectionalLightComponent>([&, pbrShader = shader](DirectionalLightComponent& lightComponent) {
					const auto& lightningAttributes = pbrShader->GetUniformBufferIfExists("LucyLightningValues");
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

				shader->BindImageHandleTo("u_ShadowMap", registry.GetImage(RGResource(ShadowImages)));

				bool imageBound = false;

				m_Scene->ViewForEach<HDRCubemapComponent>([pbrShader = shader, &registry, &imageBound](const HDRCubemapComponent& hdrComponent) {
					if (!hdrComponent.IsPrimary || imageBound)
						return;
#if USE_COMPUTE_FOR_CUBEMAP_GEN
					pbrShader->BindImageHandleTo("u_IrradianceMap", hdrComponent.GetIrradianceImage());
#else
					pbrShader->BindImageHandleTo("u_IrradianceMap", registry.GetImage(RGResource(IrradianceImage)));
#endif
					imageBound = true;
				});

				if (!shader->HasImageHandleBoundTo("u_IrradianceMap"))
					shader->BindImageHandleTo("u_IrradianceMap", Renderer::GetBlankCubeImage());

				if (auto cameraBuffer = shader->GetUniformBufferIfExists("LucyCamera")) {
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
				const auto& shader = pipeline->GetShader();

				if (auto cameraBuffer = shader->GetUniformBufferIfExists("LucyCamera")) {
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
				.ImageUsage = ImageUsage::AsColorAttachment,
				.Format = ImageFormat::R16G16_SFLOAT,
				.Layers = ShadowPass::NUM_CASCADES,
				.GenerateSampler = true,
			}, RenderPassLoadStoreAttachments::ClearStore, 
				RGResource(VSMDepth), {
				.Width = m_ShadowMapSize,
				.Height = m_ShadowMapSize,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsDepthAttachment,
				.Format = ImageFormat::D32_SFLOAT,
				.Layers = ShadowPass::NUM_CASCADES,
				.GenerateSampler = true,
			}, RenderPassLoadStoreAttachments::ClearStore);

			build.BindRenderTarget(RGResource(ShadowImages), RGResource(VSMDepth));

			const auto& editorCamera = m_Scene->GetEditorCamera();
			InitializeShadowCameras(m_ShadowMapSize, editorCamera);

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("VSMPipeline");
				const auto& depthShader = pipeline->GetShader();
				
				if (auto cameraBuffer = depthShader->GetUniformBufferIfExists("LucyCamera")) {
					ShadowCamera::ResetSplit();
					for (ShadowCamera& shadowCamera : s_ShadowCameras) {
						shadowCamera.Update();
						
						const auto& vp = shadowCamera.GetCameraViewProjection();
						cameraBuffer->Append((uint8_t*)&vp, sizeof(vp));
					}
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

		/*
		renderGraph->AddPass("VSMBlurCompute", [=, *this](RenderGraphBuilder& build) {

			build.BindRenderTarget(RGResource(ShadowImages), RGResource(VSMDepth));

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& horPipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("VSMBlurHorizontalComputePipeline");
				const auto& verPipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("VSMBlurHorizontalComputePipeline");

				Ref<Image> shadowImages = registry.GetImage(RGResource(ShadowImages));
				RenderCommand& horCmd = cmdList.BeginRenderCommand("VSMBlurHorizontalCompute");
				//draw.DownsampleImage(shadowImages, 8.0f);

				horCmd.BindPipeline(horPipeline);

				horCmd.UpdateDescriptorSets();
				horCmd.BindAllDescriptorSets();
				//horCmd.DispatchCompute();

				cmdList.EndRenderCommand();

				RenderCommand& verCmd = cmdList.BeginRenderCommand("VSMBlurVerticalCompute");

				verCmd.BindPipeline(verPipeline);

				//verCmd.UpsampleImage(shadowImages, 8.0f);
				cmdList.EndRenderCommand();
			};
		});
		*/
	}

	// The method is explained well here
	// https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-parallel-split-shadow-maps-programmable-gpus
	void ShadowPass::InitializeShadowCameras(uint32_t size, const EditorCamera& editorCamera) const {
		static constexpr float lambda = 1.0f;
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
			float C_iLog = n * std::pow(ratio, iDivM);
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
		//float radius = 0.0f;
		//for (uint32_t i = 0; i < frustumCornerCount; i++) {
		//	float distance = glm::length(frustumCornersWS[i] - frustumCenter);
		//	radius = glm::max(radius, distance);
		//}
		//radius = std::ceil(radius * 16.0f) / 16.0f;
		//
		//glm::vec3 maxExtents = glm::vec3(radius, radius, radius);
		//glm::vec3 minExtents = -maxExtents;

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

		m_Left = -radiusDistWS;
		m_Right = radiusDistWS;
		m_Bottom = -radiusDistWS;
		m_Top = radiusDistWS;
		m_NearPlane = -radiusDistWS * 6.0f;
		m_FarPlane = radiusDistWS * 6.0f;

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

			build.ReadImage(RGResource(GeometryImage));
			build.ReadImage(RGResource(GeometryDepthImage));
			build.BindRenderTarget(RGResource(GeometryImage), RGResource(GeometryDepthImage));

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("SkyboxPipeline");
				const auto& hdrSkyboxShader = pipeline->GetShader();

				bool imageBound = false;

				m_Scene->ViewForEach<HDRCubemapComponent>([shader = hdrSkyboxShader, &imageBound](const HDRCubemapComponent& hdrComponent) {
					if (!hdrComponent.IsPrimary || imageBound)
						return;
					shader->BindImageHandleTo("u_EnvironmentMap", hdrComponent.GetCubemapImage());
					imageBound = true;
				});

				if (!hdrSkyboxShader->HasImageHandleBoundTo("u_EnvironmentMap"))
					return;

				if (auto cameraBuffer = hdrSkyboxShader->GetUniformBufferIfExists("LucyCamera")) {
					const auto& vp = m_Scene->GetEditorCamera().GetCameraViewProjection();
					cameraBuffer->SetData((uint8_t*)&vp, sizeof(vp));
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
			build.SetViewportArea(HDRImageWidth, HDRImageHeight);

			build.ReadExternalTransientImage(RGResource(OriginalHDRImage));

			build.DeclareImage(RGResource(HDRLayeredImage), {
				.Width = HDRImageWidth,
				.Height = HDRImageHeight,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorTransferAttachment,
				.Format = ImageFormat::R32G32B32A32_SFLOAT,
				.Layers = 6,
				.GenerateSampler = true,
			}, RenderPassLoadStoreAttachments::DontCareDontCare);

			build.BindRenderTarget(RGResource(HDRLayeredImage));

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("HDRImageToLayeredImageConvertPipeline");
				const auto& shader = pipeline->GetShader();

				shader->BindImageHandleTo("u_EquirectangularMap", registry.GetExternalImage(RGResource(OriginalHDRImage)));

				const auto& cubeMesh = Renderer::GetEnvCubeMesh();
				const uint32_t cubeMeshIndexCount = Renderer::GetEnvCubeMeshIndexCount();

				RenderCommand& draw = cmdList.BeginRenderCommand("Cubemap Prep Draw");
				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.BindBuffers(cubeMesh);

				static const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

				VulkanPushConstant& pushConstant = shader->GetPushConstants("LucyCameraPushConstants");

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

		renderGraph->AddPass(TargetQueueFamily::Compute, "CopyToSampler2DCube", [*this](RenderGraphBuilder& build) {
			build.ReadImage(RGResource(HDRLayeredImage));
			build.WriteExternalImage(RGResource(HDRCubeImage));

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const Ref<Image>& preparedImage = registry.GetImage(RGResource(HDRLayeredImage));
				const Ref<Image>& cubeImage = registry.GetExternalImage(RGResource(HDRCubeImage));

				static constexpr uint32_t layerCount = 6;

				RenderCommand& cmd = cmdList.BeginRenderCommand("CopyToSampler2DCube");
				cmd.SetImageLayout(preparedImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, 0, 1, layerCount);
				cmd.SetImageLayout(cubeImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0, 1, layerCount);

				std::vector<VkImageCopy> regions;
				regions.reserve(layerCount);

				//copying the layered color attachment, to a sampler2DCube
				for (uint32_t face = 0; face < layerCount; face++) {
					VkImageCopy region = {
						.srcSubresource = {
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.mipLevel = 0,
							.baseArrayLayer = face,
							.layerCount = 1
						},
						.srcOffset = { 0, 0, 0 },
						.dstSubresource = {
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.mipLevel = 0,
							.baseArrayLayer = face,
							.layerCount = 1
						},
						.dstOffset = { 0, 0, 0 },
						.extent = {
							.width = HDRImageWidth,
							.height = HDRImageHeight,
							.depth = 1
						},
					};
					regions.push_back(region);
				}
				cmd.CopyImageToImage(preparedImage, cubeImage, regions);

				//cmd.SetImageLayout(preparedImage, VK_IMAGE_LAYOUT_GENERAL, 0, 0, 1, layerCount);

				cmdList.EndRenderCommand();
			};
		});

#pragma region Irradiance
#if USE_COMPUTE_FOR_CUBEMAP_GEN
		renderGraph->AddPass(TargetQueueFamily::Compute, "IrradiancePass", [*this](RenderGraphBuilder& build) {
			build.ReadExternalImage(RGResource(HDRCubeImage));
			build.ReadExternalImage(RGResource(IrradianceImage));
			build.WriteImage(RGResource(IrradianceImage));

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				static constexpr const uint32_t layerCount = 6;
				static constexpr const uint32_t workGroupSize = 8;

				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("IrradianceComputePipeline");
				const auto& shader = pipeline->GetShader();

				const auto& cubeImage = registry.GetExternalImage(RGResource(HDRCubeImage));
				const auto& irradianceImage = registry.GetExternalImage(RGResource(IrradianceImage));

				RenderCommand& draw = cmdList.BeginRenderCommand("Irradiance Draw Compute");

				draw.SetImageLayout(cubeImage, VK_IMAGE_LAYOUT_GENERAL, 0, 0, 1, layerCount);
				draw.SetImageLayout(irradianceImage, VK_IMAGE_LAYOUT_GENERAL, 0, 0, 1, layerCount);

				shader->BindImageHandleTo("u_EnvironmentMap", cubeImage);
				shader->BindImageHandleTo("u_EnvironmentIrradianceMap", irradianceImage);

				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.DispatchCompute(HDRImageWidth / workGroupSize, HDRImageHeight / workGroupSize, layerCount);

				cmdList.EndRenderCommand();
			};
		});
#else
		renderGraph->AddPass(TargetQueueFamily::Graphics, "IrradiancePass", [*this](RenderGraphBuilder& build) {
			build.SetViewportArea(HDRImageWidth, HDRImageHeight);

			build.ReadExternalImage(RGResource(HDRCubeImage));

			build.DeclareImage(RGResource(IrradianceImage), {
				.Width = HDRImageWidth,
				.Height = HDRImageHeight,
				.ImageType = ImageType::TypeCube,
				.ImageUsage = ImageUsage::AsColorAttachment,
				.Format = ImageFormat::R16G16B16A16_SFLOAT,
				.GenerateSampler = true,
				.GenerateMipmap = false,
				.ImGuiUsage = false,
			}, RenderPassLoadStoreAttachments::ClearDontCare);
			
			build.BindRenderTarget(RGResource(IrradianceImage));

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("IrradiancePipeline");
				const auto& shader = pipeline->GetShader();

				const auto& mesh = Renderer::GetEnvCubeMesh();

				RenderCommand& draw = cmdList.BeginRenderCommand("Irradiance Draw");
				const auto& irradianceImage = registry.GetImage(RGResource(IrradianceImage));
				const auto& environmentMap = registry.GetExternalImage(RGResource(HDRCubeImage));

				draw.SetImageLayout(environmentMap, VK_IMAGE_LAYOUT_GENERAL, 0, 0, 1, 6);

				shader->BindImageHandleTo("u_EnvironmentMap", environmentMap);

				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.BindBuffers(mesh);
				draw.DrawIndexed(36, 1, 0, 0, 0);

				cmdList.EndRenderCommand();
			};
		});
#endif
#pragma endregion Irradiance

#if 0
		renderGraph->AddPass("PrefilterPass", prefilterShader, [this](RenderGraphBuilder& build) {
			return [=](RenderGraphRegistry& registry, const Ref<RenderDevice>& renderDevice, RenderCommandList& cmdList) {
				CubeRenderCommand* environmentRenderCommand = (CubeRenderCommand*)command;

				static glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

				const Ref<Mesh>& cubeMesh = environmentRenderCommand->CubeMesh;

				Renderer::BindPipeline(commandBuffer, pipeline);
				Renderer::BindAllDescriptorSets(commandBuffer, pipeline);
				Renderer::BindBuffers(commandBuffer, cubeMesh);

				VulkanPushConstant& pushConstant = pipeline->GetPushConstants("LucyCameraPushConstants");

				CubePushConstantData pushConstantData;
				pushConstantData.Proj = captureProjection;
				pushConstant.SetData((uint8_t*)&pushConstantData, sizeof(CubePushConstantData));

				Renderer::BindPushConstant(commandBuffer, pipeline, pushConstant);
				Renderer::DrawIndexed(commandBuffer, cubeMesh->GetIndexBufferHandle()->GetSize(), 1, 0, 0, 0);
			};
		});
#endif
#if 0
		ComputeDispatchCommand* dispatchCommand = (ComputeDispatchCommand*)command;

		//TODO: Make this dynamic
		constexpr uint32_t maxMip = 5;
		constexpr uint32_t cubemapSize = 1024u;

		Renderer::BindPipeline(commandBuffer, pipeline);
		Renderer::BindAllDescriptorSets(commandBuffer, pipeline);

		VulkanPushConstant& pushConstant = pipeline->GetPushConstants("LucyPrefilterParams");
		const auto& environmentPrefilterMap = pipeline->GetUniformBuffers<VulkanUniformImageBuffer>("u_EnvironmentPrefilterMap");

		/*
		for (uint32_t mip = 0; mip < maxMip; mip++) {
			glm::vec4 prefilterParams = glm::vec4(cubemapSize >> mip, cubemapSize >> mip, mip / (maxMip - 1), 1.0f);
			pushConstant.SetData((uint8_t*)&prefilterParams, sizeof(glm::vec4));

			environmentPrefilterMap->BindImage(m_PrefilterImageView.GetVulkanHandle(), m_CurrentLayout, m_PrefilterImageView.GetSampler());

			Renderer::UpdateDescriptorSets(m_PrefilterComputePipeline);

			Renderer::BindPushConstant(commandBuffer, pipeline, pushConstant);
			Renderer::DispatchCompute(commandBuffer, pipeline->As<ComputePipeline>(), dispatchCommand->GetGroupCountX(), dispatchCommand->GetGroupCountY(), dispatchCommand->GetGroupCountZ());
		}
		*/
#endif
#pragma endregion CubemapPass

#pragma region BRDFPass
		//TODO:
#pragma endregion BRDFPass
	}
}