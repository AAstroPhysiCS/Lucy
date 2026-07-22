#include "lypch.h"
#include "RendererPasses.h"

#include "Scene/Scene.h"

#include "Renderer.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphBuilder.h"
#include "RenderGraph/RenderGraphRegistry.h"

#include "Device/RenderDeviceScene.h"

#include "Image/Image.h"

#include "Memory/Buffer/Buffer.h"
#include "Memory/Buffer/PushConstant.h"

#include "Pipeline/ComputePipeline.h"

#include "Scene/Components.h"

namespace Lucy {

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

			build.BindRenderTarget(RGResource(GeometryImage), RGResource(GeometryDepthImage));

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("PBRGeometryPipeline");
				const auto& settings = Renderer::GetRendererSettings();

				RenderCommand& draw = cmdList.BeginRenderCommand("PBRForwardPass");
				draw.BindPipeline(pipeline);

				uint32_t shadowImagesIndex = draw.BindImageHandleTo("TextureArrays2D_Float2", registry.GetImage(RGResource(ShadowImages)));
				uint32_t brdfImageIndex = draw.BindImageHandleTo("Textures2D_Float2", registry.GetImage(RGResource(BRDFLutImage)));

				uint32_t irradianceIndex = INVALID_INDEX;

				bool imageBound = false;

				m_Scene->ViewForEach<HDRCubemapComponent>([&draw, &registry, &irradianceIndex, &imageBound, &settings](const HDRCubemapComponent& hdrComponent) {
					if (!hdrComponent.IsPrimary || imageBound)
						return;
#if USE_COMPUTE_FOR_CUBEMAP_GEN
					irradianceIndex = draw.BindImageHandleTo("CubeTextures", hdrComponent.GetIrradianceImage());
#else
					irradianceIndex = draw.BindImageHandleTo("CubeTextures", registry.GetImage(RGResource(IrradianceImage)));
#endif
					imageBound = true;
				});

				uint32_t prefilterIndex = draw.BindImageHandleTo("CubeTextures", registry.GetImage(RGResource(PrefilterImage)));

				if (prefilterIndex == INVALID_INDEX)
					prefilterIndex = draw.BindImageHandleTo("CubeTextures", Renderer::GetBlankCubeImage());
				if (irradianceIndex == INVALID_INDEX)
					irradianceIndex = draw.BindImageHandleTo("CubeTextures", Renderer::GetBlankCubeImage());

				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();

				RenderDeviceTextureResource env[4] = {
					{ .TextureIndex = shadowImagesIndex, .SamplerIndex = 0 },
					{ .TextureIndex = prefilterIndex, .SamplerIndex = 0 },
					{ .TextureIndex = brdfImageIndex, .SamplerIndex = 0 },
					{ .TextureIndex = irradianceIndex, .SamplerIndex = 0 },
				};

				struct LocalPushConstant {
					glm::mat4 ModelMatrix;
					glm::mat4 ModelMatrixInversedTransposed;
					uint32_t MaterialID;
					RenderDeviceTextureResource PBRTextureResources[4];
					uint32_t Padding[3]{};
				};

				m_Scene->ViewRForEach<MeshComponent, TransformComponent>([&](MeshComponent& meshComponent, TransformComponent& transformComponent) {
					draw.DrawIndexedMeshWithPushConstant<LocalPushConstant>(meshComponent.GetMesh(), transformComponent.GetMatrix(),
						[env](LocalPushConstant& data, const Submesh& submesh, const glm::mat4& finalTransform) {
						data.ModelMatrix = finalTransform;
						data.ModelMatrixInversedTransposed = glm::transpose(glm::inverse(finalTransform));
						data.MaterialID = submesh.MaterialID.Index;

						memcpy(data.PBRTextureResources, env, sizeof(env));
					});
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

				RenderCommand& draw = cmdList.BeginRenderCommand("IDPass");
				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();

				struct LocalPushConstant {
					glm::mat4 ModelMatrix;
				};

				m_Scene->ViewRForEach<MeshComponent, TransformComponent>([&](MeshComponent& meshComponent, TransformComponent& transformComponent) {
					draw.DrawIndexedMeshWithPushConstant<LocalPushConstant>(meshComponent.GetMesh(), transformComponent.GetMatrix(),
						[](LocalPushConstant& data, const Submesh& submesh, const glm::mat4& finalTransform) {
						data.ModelMatrix = finalTransform;
					});
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

				RenderCommand& draw = cmdList.BeginRenderCommand("VSM Draw");
				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();

				struct LocalPushConstant {
					glm::mat4 ModelMatrix;
				};

				m_Scene->ViewRForEach<MeshComponent, TransformComponent>([&](MeshComponent& meshComponent, TransformComponent& transformComponent) {
					draw.DrawIndexedMeshWithPushConstant<LocalPushConstant>(meshComponent.GetMesh(), transformComponent.GetMatrix(), 
						[](LocalPushConstant& data, const Submesh& submesh, const glm::mat4& finalTransform) {
						data.ModelMatrix = finalTransform;
					});
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

			struct BlurData {
				int32_t BlurData[4];            // x, y, width, height
				uint32_t TextureIndicesData[4]; // x: input texture index, y: output texture index, z: sampler index, w: unused
			};

			if (direction == GaussianBlurDirection::Horizontal) {
				uint32_t inputIndex = cmd.BindImageHandleTo("TextureArrays2D_Float2", shadowImages);
				uint32_t outputIndex = cmd.BindImageHandleTo("StorageTextureArrays2D_Float2", shadowImagesBlurred);
				uint32_t samplerIndex = 0; //TODO:

				BlurData data = {
					.BlurData = { 1, 0, width, height },
					.TextureIndicesData = { inputIndex, outputIndex, samplerIndex, 0 }
				};

				pushConstant.SetData(reinterpret_cast<uint8_t*>(&data), sizeof(data));
			} else {
				uint32_t inputIndex = cmd.BindImageHandleTo("TextureArrays2D_Float2", shadowImagesBlurred);
				uint32_t outputIndex = cmd.BindImageHandleTo("StorageTextureArrays2D_Float2", shadowImages);
				uint32_t samplerIndex = 0;

				BlurData data = {
					.BlurData = { 0, 1, width, height },
					.TextureIndicesData = { inputIndex, outputIndex, samplerIndex, 0 }
				};

				pushConstant.SetData(reinterpret_cast<uint8_t*>(&data), sizeof(data));
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

		s_ShadowCameras.clear();
		s_ShadowCameras.reserve(NUM_CASCADES);

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

		float radiusDistWS = std::ceil(glm::length(frustumCornersWS[0] - frustumCornersWS[6]) * 16.0f) / 2.0f;

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

		UpdateProjection();

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
				const Ref<Mesh>& cubeMesh = Renderer::GetEnvCubeMesh();

				RenderCommand& draw = cmdList.BeginRenderCommand("Skybox Draw");
				draw.BindPipeline(pipeline);

				uint32_t index = INVALID_INDEX;

				m_Scene->ViewForEach<HDRCubemapComponent>([&imageBound, &index, &draw, &registry](const HDRCubemapComponent& hdrComponent) {
					if (!hdrComponent.IsPrimary || imageBound)
						return;
					index = draw.BindImageHandleTo("CubeTextures", registry.GetImage(RGResource(PrefilterImage)));
					imageBound = true;
				});

				if (!imageBound) {
					cmdList.EndRenderCommand();
					return;
				}

				struct LocalPushConstant {
					glm::uvec3 Data; // x: environment map index, y: sampler index, z: mip level
				};

				GlobalPushConstant<LocalPushConstant> pushConstantData {
					draw.GetGlobalBufferAddress(),
					0,
					{ .Data = { index, 0, settings.EnvironmentLOD } }
				};

				auto& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.BindPushConstant(pushConstant);
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
				static const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

				struct LocalPushConstant {
					glm::mat4 CaptureProjection;
					glm::vec2 Data;
				};

				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("HDRImageToLayeredImageConvertPipeline");

				const auto& cubeMesh = Renderer::GetEnvCubeMesh();
				const uint32_t cubeMeshIndexCount = Renderer::GetEnvCubeMeshIndexCount();

				RenderCommand& draw = cmdList.BeginRenderCommand("Cubemap Prep Draw");
				draw.BindPipeline(pipeline);

				uint32_t originalHDRImageIndex = draw.BindImageHandleTo("Textures2D", registry.GetImage(RGResource(OriginalHDRImage)));

				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.BindBuffers(cubeMesh);

				GlobalPushConstant<LocalPushConstant> localPushConstant{
					.Root = draw.GetGlobalBufferAddress(),
					.Data = {
						.CaptureProjection = captureProjection, 
						.Data = { originalHDRImageIndex, 0 } 
					}
				};

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");

				ByteBuffer pushConstantData;
				pushConstantData.SetData((uint8_t*)&localPushConstant, sizeof(localPushConstant));
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
				static const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
				
				static constexpr const uint32_t layerCount = 6;
				static constexpr const uint32_t workGroupSize = 8;

				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("IrradianceComputePipeline");

				const auto& cubeImage = registry.GetImage(RGResource(HDRLayeredImage));
				const auto& irradianceImage = registry.GetImage(RGResource(IrradianceImage));

				RenderCommand& draw = cmdList.BeginRenderCommand("Irradiance Draw Compute");
				draw.BindPipeline(pipeline);

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");

				uint32_t layeredImageIndex = draw.BindImageHandleTo("CubeTextures", cubeImage);
				uint32_t irradianceImageIndex = draw.BindImageHandleTo("StorageTextureArrays2D", irradianceImage);

				struct LocalPushConstant {
					glm::mat4 ProjMatrix;
					glm::vec4 Data; // x: size, y: sampler index, z: environment map index, w: environment irradiance compute index
				};

				GlobalPushConstant<LocalPushConstant> pushConstantData{
					.Root = draw.GetGlobalBufferAddress(),
					.Data = {
						.ProjMatrix = captureProjection,
						.Data = { m_Size, 0, layeredImageIndex, irradianceImageIndex, }
					}
				};

				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				//draw.SetImageLayout(cubeImage, VK_IMAGE_LAYOUT_GENERAL, 0, 0, 1, layerCount);
				//draw.SetImageLayout(irradianceImage, VK_IMAGE_LAYOUT_GENERAL, 0, 0, 1, layerCount);

				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.BindPushConstant(pushConstant);
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

				pipeline->BindGlobalImageHandleTo("u_EnvironmentMap", environmentMap);

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

				static constexpr uint32_t workGroupSize = 8;
				
				struct LocalPushConstant {
					glm::vec4 PrefilterParams; // x: sizeX, y: sizeY, z: roughness, w: mip
					glm::uvec3 TextureData;    // x: environment map index, y: prefilter map index, z: sampler index
				};

				RenderCommand& cmd = cmdList.BeginRenderCommand("Prefilter Draw Compute");
				cmd.BindPipeline(pipeline);

				uint32_t layeredImageIndex = cmd.BindImageHandleTo("CubeTextures", registry.GetImage(RGResource(HDRLayeredImage)));

				std::array<uint32_t, MAX_MIP_LEVELS> prefilterImageIndices{};
				for (uint32_t mip = 0; mip < MAX_MIP_LEVELS; mip++) {
					prefilterImageIndices[mip] = cmd.BindImageHandleTo("StorageTextureArrays2D", registry.GetImage(RGResource(PrefilterImage)), mip);
				}

				cmd.UpdateDescriptorSets();
				cmd.BindAllDescriptorSets();

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				for (uint32_t mip = 0; mip < MAX_MIP_LEVELS; mip++) {
					const uint32_t mipSize = std::max(1u, m_CubemapSize >> mip);

					GlobalPushConstant<LocalPushConstant> params{
						.Root = cmd.GetGlobalBufferAddress(),
						.Data = {
							.PrefilterParams = glm::vec4(mipSize, mipSize, mip / float(MAX_MIP_LEVELS - 1), mip),
							.TextureData = { layeredImageIndex, prefilterImageIndices[mip], 0}
						}
					};

					pushConstant.SetData((uint8_t*)&params, sizeof(params));

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
				static constexpr uint32_t workGroupSize = 8;

				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("BRDFLutComputePipeline");
				const auto& brdfLutImage = registry.GetImage(RGResource(BRDFLutImage));

				RenderCommand& draw = cmdList.BeginRenderCommand("BRDFLut Draw Compute");
				draw.BindPipeline(pipeline);

				struct LocalPushConstant {
					glm::uvec4 u_BRDFData;           // x: width, y: height, z: input texture index, w: unused
				};

				uint32_t brdfIndex = draw.BindImageHandleTo("StorageTextures2D_Float2", brdfLutImage);

				LocalPushConstant pushConstantData {
					{ m_Size, m_Size, brdfIndex, 0 }
				};

				auto& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				draw.BindPushConstant(pushConstant);
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