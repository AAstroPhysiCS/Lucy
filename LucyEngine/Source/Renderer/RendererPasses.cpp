#include "lypch.h"
#include "RendererPasses.h"

#include "Scene/Scene.h"

#include "Renderer.h"

#include "Mesh.h"
#include "MeshFactory.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphBuilder.h"
#include "RenderGraph/RenderGraphRegistry.h"

#include "Device/RenderDeviceScene.h"

#include "Image/Image.h"

#include "Memory/Buffer/Buffer.h"
#include "Memory/Buffer/PushConstant.h"

#include "Pipeline/ComputePipeline.h"
#include "Pipeline/RayTracingPipeline.h"

#include "Scene/Components.h"

namespace Lucy {

#pragma region GPUDrivenRendererPasses

	GPUDrivenRendererPass::GPUDrivenRendererPass(Ref<RenderDevice> device) {
		const auto& scene = device->GetScene();

		Renderer::ImportExternalRenderGraphResource(RGResource(GPUVerticesBuffer), scene->GetGlobalVertexBufferHandle(), RGBufferData{ .InFlightMode = false });
		Renderer::ImportExternalRenderGraphResource(RGResource(GPUIndicesBuffer), scene->GetGlobalIndexBufferHandle(), RGBufferData{ .InFlightMode = false });
		Renderer::ImportExternalRenderGraphResource(RGResource(GPUSceneBuffer), scene->GetCurrentFrameBufferHandles("GPUScene"), RGBufferData { .InFlightMode = true });
		Renderer::ImportExternalRenderGraphResource(RGResource(GPUObjectsBuffer), scene->GetCurrentFrameBufferHandles("GPUObjects"), RGBufferData{ .InFlightMode = true });
		Renderer::ImportExternalRenderGraphResource(RGResource(GPUMeshesBuffer), scene->GetCurrentFrameBufferHandles("GPUMeshes"), RGBufferData{ .InFlightMode = true });
		Renderer::ImportExternalRenderGraphResource(RGResource(GPUMeshLODsBuffer), scene->GetCurrentFrameBufferHandles("GPUMeshLODs"), RGBufferData{ .InFlightMode = true });
		Renderer::ImportExternalRenderGraphResource(RGResource(GPUSubmeshesBuffer), scene->GetCurrentFrameBufferHandles("GPUSubmeshes"), RGBufferData{ .InFlightMode = true });
		Renderer::ImportExternalRenderGraphResource(RGResource(GPUMeshletsBuffer), scene->GetCurrentFrameBufferHandles("GPUMeshlets"), RGBufferData{ .InFlightMode = true });
		Renderer::ImportExternalRenderGraphResource(RGResource(GPUCullViewsBuffer), scene->GetCurrentFrameBufferHandles("GPUCullViews"), RGBufferData{ .InFlightMode = true });
	}

	void GPUDrivenRendererPass::AddPass(const Ref<RenderGraph>& renderGraph) {
		AddObjectCullPass(renderGraph);
		AddSubmeshDispatchBuildPass(renderGraph);
		AddSubmeshCullPass(renderGraph);
		AddMeshletDispatchBuildPass(renderGraph);
		AddMeshletCullPass(renderGraph);
		AddHiZPass(renderGraph);

		s_GPUCullPushConstants.resize(Renderer::GetMaxFramesInFlight());
		for (uint32_t frameIndex = 0; frameIndex < Renderer::GetMaxFramesInFlight(); frameIndex++) {
			s_GPUCullPushConstants[frameIndex] = CreateGPUCullPushConstant(renderGraph->GetRegistry(), 0, frameIndex);
		}
	}

	GlobalPushConstant<RenderDeviceGPUCullData> GPUDrivenRendererPass::CreateGPUCullPushConstant(const RenderGraphRegistry& registry, uint32_t viewIndex, uint32_t frameIndex) {
		GlobalPushConstant<RenderDeviceGPUCullData> pushConstantData{
			.Root = registry.GetBuffer(RGResource(GPUSceneBuffer), frameIndex)->GetDeviceAddress(),
			.Data = {
				.VisibleObjects = registry.GetBuffer(RGResource(VisibleObjects), frameIndex)->GetDeviceAddress(),
				.VisibleObjectCount = registry.GetBuffer(RGResource(VisibleObjectsCount), frameIndex)->GetDeviceAddress(),
				.VisibleSubmeshes = registry.GetBuffer(RGResource(VisibleSubmeshes), frameIndex)->GetDeviceAddress(),
				.VisibleSubmeshCount = registry.GetBuffer(RGResource(VisibleSubmeshesCount), frameIndex)->GetDeviceAddress(),
				.SubmeshDispatchIndirect = registry.GetBuffer(RGResource(SubmeshDispatch), frameIndex)->GetDeviceAddress(),
				.MeshletDispatchIndirect = registry.GetBuffer(RGResource(MeshletDispatch), frameIndex)->GetDeviceAddress(),
				.VisibleDraws = registry.GetBuffer(RGResource(VisibleDraws), frameIndex)->GetDeviceAddress(),
				.IndirectCommands = registry.GetBuffer(RGResource(IndirectCommands), frameIndex)->GetDeviceAddress(),
				.DrawCounts = registry.GetBuffer(RGResource(DrawCounts), frameIndex)->GetDeviceAddress(),
				.ObjectCapacity = static_cast<uint32_t>(RenderDeviceScene::GetObjectCapacity()),
				.SubmeshCapacity = static_cast<uint32_t>(RenderDeviceScene::GetSubmeshCapacity()),
				.CommandCapacityPerBin = static_cast<uint32_t>(RenderDeviceScene::GetMeshletCapacity()),
				.ViewIndex = viewIndex,
				.RenderBinCount = RenderBin::Count
			}
		};

		return pushConstantData;
	}

	// Idea from: https://medium.com/@mil_kru/two-pass-occlusion-culling-4100edcad501
	void GPUDrivenRendererPass::AddHiZPass(const Ref<RenderGraph>& renderGraph) {
		/*renderGraph->AddPass(TargetQueueFamily::Compute, "HiZPass", [=](RenderGraphBuilder& build) {
			build.DeclareImage(RGResource(HiZImage), {
				.Width = 2048,
				.Height = 2048,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
				.Format = ImageFormat::R32_SFLOAT,
				.GenerateSampler = true,
				.ImGuiUsage = true,
			}, RenderPassLoadStoreAttachments::ClearStore);

			build.WriteImage(RGResource(HiZImage), RenderGraphResourceAccess::StorageWrite);

			return [=](RenderGraphRegistry& registry, RenderCommandList& cmdList) {

			};
		});*/
	}

	void GPUDrivenRendererPass::AddSubmeshDispatchBuildPass(const Ref<RenderGraph>& renderGraph) {
		renderGraph->AddPass(TargetQueueFamily::Compute, "SubmeshDispatchBuildPass", [=](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.DeclareBuffer(RGResource(SubmeshDispatch), {
				.DebugName = "SubmeshDispatch",
				.Size = sizeof(VkDispatchIndirectCommand),
				.Usage = BufferUsage::Storage | BufferUsage::Indirect
			});

			build.ReadBuffer(RGResource(VisibleObjectsCount), RenderGraphResourceAccess::StorageRead);
			build.WriteBuffer(RGResource(SubmeshDispatch), RenderGraphResourceAccess::StorageWrite);

			return [=](RenderGraphRegistry& registry, RenderCommand& command) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::SubmeshDispatchBuildPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("GPUBuildSubmeshDispatchPipeline");

				command.BindPipeline(pipeline);	

				auto& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				auto& pushConstantData = s_GPUCullPushConstants[Renderer::GetCurrentFrameIndex()];
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				command.BindPushConstant(pushConstant);
				command.DispatchCompute(1, 1, 1);
			};
		});
	}

	void GPUDrivenRendererPass::AddSubmeshCullPass(const Ref<RenderGraph>& renderGraph) {
		renderGraph->AddPass(TargetQueueFamily::Compute, "SubmeshCullPass", [=](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.DeclareBuffer(RGResource(VisibleSubmeshes), {
				.DebugName = "VisibleSubmeshes",
				.Size = RenderDeviceScene::GetSubmeshCapacity() * sizeof(RenderDeviceVisibleSubmeshData),
				.Usage = BufferUsage::Storage
			});

			build.ReadExternalBuffer(RGResource(GPUSceneBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUObjectsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUMeshesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUMeshLODsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUSubmeshesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUCullViewsBuffer), RenderGraphResourceAccess::StorageRead);

			build.ReadBuffer(RGResource(VisibleObjects), RenderGraphResourceAccess::StorageRead);
			build.ReadBuffer(RGResource(VisibleObjectsCount), RenderGraphResourceAccess::StorageRead);
			build.ReadBuffer(RGResource(SubmeshDispatch), RenderGraphResourceAccess::IndirectRead);

			build.WriteBuffer(RGResource(VisibleSubmeshes), RenderGraphResourceAccess::StorageWrite);
			build.WriteBuffer(RGResource(VisibleSubmeshesCount), RenderGraphResourceAccess::StorageReadWrite);

			return [=](RenderGraphRegistry& registry, RenderCommand& command) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::SubmeshCullPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("GPUCullSubmeshesPipeline");
				command.BindPipeline(pipeline);

				auto& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				auto& pushConstantData = s_GPUCullPushConstants[Renderer::GetCurrentFrameIndex()];
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				command.BindPushConstant(pushConstant);
				command.DispatchComputeIndirect(registry.GetBuffer(RGResource(SubmeshDispatch)), 0);
			};
		});
	}

	void GPUDrivenRendererPass::AddMeshletDispatchBuildPass(const Ref<RenderGraph>& renderGraph) {
		renderGraph->AddPass(TargetQueueFamily::Compute, "MeshletDispatchBuildPass", [=](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.DeclareBuffer(RGResource(MeshletDispatch), {
				.DebugName = "MeshletDispatch",
				.Size = sizeof(VkDispatchIndirectCommand),
				.Usage = BufferUsage::Storage | BufferUsage::Indirect
			});

			build.ReadBuffer(RGResource(VisibleSubmeshesCount), RenderGraphResourceAccess::StorageRead);
			build.WriteBuffer(RGResource(MeshletDispatch), RenderGraphResourceAccess::StorageWrite);

			return [=](RenderGraphRegistry& registry, RenderCommand& command) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::MeshletDispatchBuildPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("GPUBuildMeshletDispatchPipeline");
/**/
				command.BindPipeline(pipeline);

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				auto& pushConstantData = s_GPUCullPushConstants[Renderer::GetCurrentFrameIndex()];
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				command.BindPushConstant(pushConstant);
				command.DispatchCompute(1, 1, 1);
			};
		});
	}

	void GPUDrivenRendererPass::AddObjectCullPass(const Ref<RenderGraph>& renderGraph) {
		renderGraph->AddPass(TargetQueueFamily::Compute, "ResetPass", [=](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.DeclareBuffer(RGResource(VisibleObjectsCount), {
				.DebugName = "VisibleObjectsCount",
				.Size = sizeof(uint32_t),
				.Usage = BufferUsage::Storage | BufferUsage::TransferDestination,
			});

			build.DeclareBuffer(RGResource(VisibleSubmeshesCount), {
				.DebugName = "VisibleSubmeshesCount",
				.Size = sizeof(uint32_t),
				.Usage = BufferUsage::Storage | BufferUsage::TransferDestination
			});

			build.DeclareBuffer(RGResource(DrawCounts), {
				.DebugName = "DrawCounts",
				.Size = RenderBin::Count * sizeof(uint32_t),
				.Usage = BufferUsage::Storage | BufferUsage::TransferDestination | BufferUsage::Indirect
			});

			build.WriteBuffer(RGResource(VisibleObjectsCount), RenderGraphResourceAccess::TransferWrite);
			build.WriteBuffer(RGResource(VisibleSubmeshesCount), RenderGraphResourceAccess::TransferWrite);
			build.WriteBuffer(RGResource(DrawCounts), RenderGraphResourceAccess::TransferWrite);

			return [=](RenderGraphRegistry& registry, RenderCommand& command) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::ResetPass");
				command.FillBuffer(registry.GetBuffer(RGResource(VisibleObjectsCount)), 0, sizeof(uint32_t), 0);
				command.FillBuffer(registry.GetBuffer(RGResource(VisibleSubmeshesCount)), 0, sizeof(uint32_t), 0);
				command.FillBuffer(registry.GetBuffer(RGResource(DrawCounts)), 0, RenderBin::Count * sizeof(uint32_t), 0);
			};
		});

		renderGraph->AddPass(TargetQueueFamily::Compute, "ObjectCullPass", [=](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.DeclareBuffer(RGResource(VisibleObjects), {
				.DebugName = "VisibleObjects",
				.Size = RenderDeviceScene::GetObjectCapacity() * sizeof(RenderDeviceVisibleObjectData),
				.Usage = BufferUsage::Storage
			});

			build.ReadExternalBuffer(RGResource(GPUSceneBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUObjectsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUMeshesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUMeshLODsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUCullViewsBuffer), RenderGraphResourceAccess::StorageRead);

			build.ReadBuffer(RGResource(VisibleObjectsCount), RenderGraphResourceAccess::StorageRead);

			build.WriteBuffer(RGResource(VisibleObjects), RenderGraphResourceAccess::StorageWrite);
			build.WriteBuffer(RGResource(VisibleObjectsCount), RenderGraphResourceAccess::StorageReadWrite);

			return [=](RenderGraphRegistry& registry, RenderCommand& command) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::ObjectCullPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("GPUCullObjectsPipeline");

				command.BindPipeline(pipeline);

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				auto& pushConstantData = s_GPUCullPushConstants[Renderer::GetCurrentFrameIndex()];
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				command.BindPushConstant(pushConstant);
				command.DispatchCompute((RenderDeviceScene::GetObjectCapacity() + 63) / 64, 1, 1);
			};
		});
	}

	void GPUDrivenRendererPass::AddMeshletCullPass(const Ref<RenderGraph>& renderGraph) {
		renderGraph->AddPass(TargetQueueFamily::Compute, "MeshletCullPass", [=](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);
			
			build.DeclareBuffer(RGResource(VisibleDraws), {
				.DebugName = "VisibleDraws",
				.Size = RenderBin::Count * RenderDeviceScene::GetMeshletCapacity() * sizeof(RenderDeviceVisibleDrawData),
				.Usage = BufferUsage::Storage | BufferUsage::TransferDestination
			});

			build.DeclareBuffer(RGResource(IndirectCommands), {
				.DebugName = "IndirectCommands",
				.Size = RenderBin::Count * RenderDeviceScene::GetMeshletCapacity() * sizeof(VkDrawIndexedIndirectCommand),
				.Usage = BufferUsage::Storage | BufferUsage::TransferDestination | BufferUsage::Indirect
			});

			build.ReadExternalBuffer(RGResource(GPUObjectsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUSubmeshesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUMeshletsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUCullViewsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUMeshLODsBuffer), RenderGraphResourceAccess::StorageRead);

			build.ReadBuffer(RGResource(VisibleSubmeshes), RenderGraphResourceAccess::StorageRead);
			build.ReadBuffer(RGResource(VisibleSubmeshesCount), RenderGraphResourceAccess::StorageRead);
			build.ReadBuffer(RGResource(MeshletDispatch), RenderGraphResourceAccess::IndirectRead);

			build.WriteBuffer(RGResource(VisibleDraws), RenderGraphResourceAccess::StorageWrite);
			build.WriteBuffer(RGResource(IndirectCommands), RenderGraphResourceAccess::StorageWrite);
			build.WriteBuffer(RGResource(DrawCounts), RenderGraphResourceAccess::StorageReadWrite);

			return [=](RenderGraphRegistry& registry, RenderCommand& command) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::MeshletCullPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("GPUCullMeshletsPipeline");

				command.BindPipeline(pipeline);

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				auto& pushConstantData = s_GPUCullPushConstants[Renderer::GetCurrentFrameIndex()];
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				command.BindAllDescriptorSets();
				command.BindPushConstant(pushConstant);
				command.DispatchComputeIndirect(registry.GetBuffer(RGResource(MeshletDispatch)), 0);
			};
		});
	}

#pragma endregion GPUDrivenRendererPasses

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
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
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

			build.DeclareImage(RGResource(ObjectIDImage), {
				.Width = m_Width,
				.Height = m_Height,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorTransferAttachment,
				.Format = ImageFormat::R32_UINT,
				.GenerateSampler = false
			}, RenderPassLoadStoreAttachments::ClearStore);

			build.ReadImage(RGResource(ShadowImagesFinal), RenderGraphResourceAccess::ShaderSampledRead);
			build.ReadExternalImage(RGResource(BRDFLutImage), RenderGraphResourceAccess::ShaderSampledRead);

			build.ReadExternalBuffer(RGResource(GPUSceneBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUObjectsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUSubmeshesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUVerticesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUIndicesBuffer), RenderGraphResourceAccess::IndexRead);

			build.ReadBuffer(RGResource(VisibleDraws), RenderGraphResourceAccess::StorageRead);
			build.ReadBuffer(RGResource(IndirectCommands), RenderGraphResourceAccess::IndirectRead);
			build.ReadBuffer(RGResource(DrawCounts), RenderGraphResourceAccess::IndirectRead);

			build.BindRenderTarget(RGResource(GeometryImage), RGResource(GeometryDepthImage));
			build.BindRenderTarget(RGResource(ObjectIDImage));

			return [=](RenderGraphRegistry& registry, RenderCommand& draw) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::PBRGeometryPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("PBRGeometryPipeline");
				const auto& settings = Renderer::GetRendererSettings();

				draw.BindPipeline(pipeline);

				uint32_t shadowImagesIndex = draw.BindImageHandleTo("TextureArrays2D_Float2", registry.GetImage(RGResource(ShadowImagesFinal)));
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

				struct LocalPushConstant {
					RenderDeviceBufferReference VisibleDraws = 0;
					RenderDeviceTextureResource PBRTextureResources[4];
				};

				GlobalPushConstant<LocalPushConstant> pushConstantData{
					.Root = registry.GetBuffer(RGResource(GPUSceneBuffer))->GetDeviceAddress(),
					.Data = {
						.VisibleDraws = registry.GetBuffer(RGResource(VisibleDraws))->GetDeviceAddress(),
						.PBRTextureResources = {
							{ .TextureIndex = shadowImagesIndex, .SamplerIndex = draw.GetLinearRepeatSampler() },
							{ .TextureIndex = prefilterIndex, .SamplerIndex = draw.GetLinearRepeatSampler() },
							{ .TextureIndex = brdfImageIndex, .SamplerIndex = draw.GetLinearRepeatSampler() },
							{ .TextureIndex = irradianceIndex, .SamplerIndex = draw.GetLinearRepeatSampler() },
						}
					}
				};

				const uint32_t commandCapacityPerBin = RenderDeviceScene::GetMeshletCapacity();

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				draw.BindPushConstant(pushConstant);
				draw.BindBuffers(registry.GetBuffer(RGResource(GPUIndicesBuffer)));

				for (uint32_t renderBin = 0; renderBin < RenderBin::Count; renderBin++) {
					uint64_t commandOffset = static_cast<uint64_t>(renderBin) * commandCapacityPerBin * sizeof(VkDrawIndexedIndirectCommand);
					uint64_t countOffset = static_cast<uint64_t>(renderBin) * sizeof(uint32_t);

					draw.DrawIndexedIndirectCount(registry.GetBuffer(RGResource(IndirectCommands)), commandOffset, 
						registry.GetBuffer(RGResource(DrawCounts)), countOffset, commandCapacityPerBin, sizeof(VkDrawIndexedIndirectCommand));
				}
			};
		});
	}

#pragma endregion ForwardPBRPass

#pragma region ShadowPass

	ShadowPass::ShadowPass(Ref<RenderDevice> device, Ref<Scene> scene, uint32_t size)
		: m_Device(device), m_Scene(scene), m_ShadowMapSize(size) {
		ShadowCamera::ResetSplit();
		InitializeShadowCameras(m_ShadowMapSize, m_Scene->GetEditorCamera());
		for (ShadowCamera& shadowCamera : s_ShadowCameras)
			shadowCamera.CreateCullView(m_Device);
	}

	GlobalPushConstant<RenderDeviceGPUShadowCullData> ShadowPass::CreateGPUCullPushConstant(const RenderGraphRegistry& registry, uint32_t frameIndex) {
		return {
			.Root = registry.GetBuffer(RGResource(GPUSceneBuffer), frameIndex)->GetDeviceAddress(),
			.Data = {
				.VisibleObjects = registry.GetBuffer(RGResource(ShadowVisibleObjects), frameIndex)->GetDeviceAddress(),
				.VisibleObjectCount = registry.GetBuffer(RGResource(ShadowVisibleObjectCount), frameIndex)->GetDeviceAddress(),
				.VisibleSubmeshes = registry.GetBuffer(RGResource(ShadowVisibleSubmeshes), frameIndex)->GetDeviceAddress(),
				.VisibleSubmeshCount = registry.GetBuffer(RGResource(ShadowVisibleSubmeshCount), frameIndex)->GetDeviceAddress(),
				.SubmeshDispatchIndirect = registry.GetBuffer(RGResource(ShadowSubmeshDispatches), frameIndex)->GetDeviceAddress(),
				.MeshletDispatchIndirect = registry.GetBuffer(RGResource(ShadowMeshletDispatches), frameIndex)->GetDeviceAddress(),
				.VisibleDraws = registry.GetBuffer(RGResource(ShadowVisibleDraws), frameIndex)->GetDeviceAddress(),
				.IndirectCommands = registry.GetBuffer(RGResource(ShadowIndirectCommands), frameIndex)->GetDeviceAddress(),
				.DrawCount = registry.GetBuffer(RGResource(ShadowDrawCounts), frameIndex)->GetDeviceAddress(),
				.ViewIndices = {
					s_ShadowCameras[0].GetCullViewHandle().Index,
					s_ShadowCameras[1].GetCullViewHandle().Index,
					s_ShadowCameras[2].GetCullViewHandle().Index,
					s_ShadowCameras[3].GetCullViewHandle().Index
				},
				.Data = {
					static_cast<uint32_t>(RenderDeviceScene::GetObjectCapacity()),
					static_cast<uint32_t>(RenderDeviceScene::GetMeshletCapacity()),
					static_cast<uint32_t>(RenderDeviceScene::GetSubmeshCapacity()),
					RenderDeviceSceneGlobalData::NUM_CASCADES
				}
			}
		};
	}

	void ShadowPass::AddPass(const Ref<RenderGraph>& renderGraph) {
		uint32_t objectCapacity = RenderDeviceScene::GetObjectCapacity();
		uint32_t commandCapacity = RenderDeviceScene::GetMeshletCapacity();
		uint64_t submeshCapacity = RenderDeviceScene::GetSubmeshCapacity();

		renderGraph->AddPass(TargetQueueFamily::Compute, "ShadowResetPass", [=](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.DeclareBuffer(RGResource(ShadowVisibleObjects), {
				.DebugName = "ShadowVisibleObjects",
				.Size = objectCapacity * sizeof(RenderDeviceVisibleObjectData),
				.Usage = BufferUsage::Storage
			});

			build.DeclareBuffer(RGResource(ShadowVisibleObjectCount), {
				.DebugName = "ShadowVisibleObjectCount",
				.Size = sizeof(uint32_t),
				.Usage = BufferUsage::Storage | BufferUsage::TransferDestination
			});

			build.DeclareBuffer(RGResource(ShadowVisibleSubmeshes), {
				.DebugName = "ShadowVisibleSubmeshes",
				.Size = submeshCapacity * sizeof(RenderDeviceVisibleSubmeshData),
				.Usage = BufferUsage::Storage
			});

			build.DeclareBuffer(RGResource(ShadowVisibleSubmeshCount), {
				.DebugName = "ShadowVisibleSubmeshCount",
				.Size = sizeof(uint32_t),
				.Usage = BufferUsage::Storage | BufferUsage::TransferDestination
			});

			build.DeclareBuffer(RGResource(ShadowSubmeshDispatches), {
				.DebugName = "ShadowSubmeshDispatches",
				.Size = sizeof(VkDispatchIndirectCommand),
				.Usage = BufferUsage::Storage | BufferUsage::Indirect
			});

			build.DeclareBuffer(RGResource(ShadowMeshletDispatches), {
				.DebugName = "ShadowMeshletDispatches",
				.Size = sizeof(VkDispatchIndirectCommand),
				.Usage = BufferUsage::Storage | BufferUsage::Indirect
			});

			build.DeclareBuffer(RGResource(ShadowVisibleDraws), {
				.DebugName = "ShadowVisibleDraws",
				.Size = commandCapacity * sizeof(RenderDeviceVisibleDrawData),
				.Usage = BufferUsage::Storage
			});

			build.DeclareBuffer(RGResource(ShadowIndirectCommands), {
				.DebugName = "ShadowIndirectCommands",
				.Size = commandCapacity * sizeof(VkDrawIndexedIndirectCommand),
				.Usage = BufferUsage::Storage | BufferUsage::Indirect
			});

			build.DeclareBuffer(RGResource(ShadowDrawCounts), {
				.DebugName = "ShadowDrawCounts",
				.Size = sizeof(uint32_t),
				.Usage = BufferUsage::Storage | BufferUsage::TransferDestination | BufferUsage::Indirect
			});

			build.WriteBuffer(RGResource(ShadowVisibleObjectCount), RenderGraphResourceAccess::TransferWrite);
			build.WriteBuffer(RGResource(ShadowVisibleSubmeshCount), RenderGraphResourceAccess::TransferWrite);
			build.WriteBuffer(RGResource(ShadowDrawCounts), RenderGraphResourceAccess::TransferWrite);

			return [=](RenderGraphRegistry& registry, RenderCommand& command) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::ShadowResetPass");
				command.FillBuffer(registry.GetBuffer(RGResource(ShadowVisibleObjectCount)), 0, sizeof(uint32_t), 0);
				command.FillBuffer(registry.GetBuffer(RGResource(ShadowVisibleSubmeshCount)), 0, sizeof(uint32_t), 0);
				command.FillBuffer(registry.GetBuffer(RGResource(ShadowDrawCounts)), 0, sizeof(uint32_t), 0);

				s_ShadowCullPushConstants.resize(Renderer::GetMaxFramesInFlight());
				for (uint32_t frameIndex = 0; frameIndex < Renderer::GetMaxFramesInFlight(); frameIndex++) {
					s_ShadowCullPushConstants[frameIndex] = CreateGPUCullPushConstant(renderGraph->GetRegistry(), frameIndex);
				}
			};
		});

		renderGraph->AddPass(TargetQueueFamily::Compute, "ShadowObjectCullPass", [=, *this](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.ReadExternalBuffer(RGResource(GPUSceneBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUObjectsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUMeshesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUCullViewsBuffer), RenderGraphResourceAccess::StorageRead);

			build.ReadBuffer(RGResource(ShadowVisibleObjectCount), RenderGraphResourceAccess::StorageReadWrite);

			build.WriteBuffer(RGResource(ShadowVisibleObjects), RenderGraphResourceAccess::StorageWrite);
			build.WriteBuffer(RGResource(ShadowVisibleObjectCount), RenderGraphResourceAccess::StorageReadWrite);

			return [=](RenderGraphRegistry& registry, RenderCommand& command) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::ShadowObjectCullPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("GPUCullShadowObjectsPipeline");

				command.BindPipeline(pipeline);

				auto& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				auto& pushConstantData = s_ShadowCullPushConstants[Renderer::GetCurrentFrameIndex()];
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				command.BindPushConstant(pushConstant);
				command.DispatchCompute((objectCapacity + 63) / 64, 1, 1);
			};
		});

		renderGraph->AddPass(TargetQueueFamily::Compute, "ShadowSubmeshDispatchBuildPass", [=, *this](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.ReadBuffer(RGResource(ShadowVisibleObjectCount), RenderGraphResourceAccess::StorageRead);
			build.WriteBuffer(RGResource(ShadowSubmeshDispatches), RenderGraphResourceAccess::StorageWrite);

			return [=, *this](RenderGraphRegistry& registry, RenderCommand& command) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::ShadowSubmeshDispatchBuildPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("GPUBuildShadowSubmeshDispatchPipeline");

				command.BindPipeline(pipeline);

				auto& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				auto& pushConstantData = s_ShadowCullPushConstants[Renderer::GetCurrentFrameIndex()];
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				command.BindPushConstant(pushConstant);
				command.DispatchCompute(1, 1, 1);
			};
		});

		renderGraph->AddPass(TargetQueueFamily::Compute, "ShadowSubmeshCullPass", [=, *this](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.ReadExternalBuffer(RGResource(GPUSceneBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUObjectsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUMeshesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUCullViewsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUMeshLODsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUSubmeshesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUCullViewsBuffer), RenderGraphResourceAccess::StorageRead);

			build.ReadBuffer(RGResource(ShadowVisibleObjects), RenderGraphResourceAccess::StorageRead);
			build.ReadBuffer(RGResource(ShadowVisibleObjectCount), RenderGraphResourceAccess::StorageRead);
			build.ReadBuffer(RGResource(ShadowSubmeshDispatches), RenderGraphResourceAccess::IndirectRead);

			build.WriteBuffer(RGResource(ShadowVisibleSubmeshes), RenderGraphResourceAccess::StorageWrite);
			build.WriteBuffer(RGResource(ShadowVisibleSubmeshCount), RenderGraphResourceAccess::StorageReadWrite);

			return [=, *this](RenderGraphRegistry& registry, RenderCommand& command) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::ShadowSubmeshCullPass");

				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("GPUCullShadowSubmeshesPipeline");
				command.BindPipeline(pipeline);

				auto& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				auto& pushConstantData = s_ShadowCullPushConstants[Renderer::GetCurrentFrameIndex()];
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				command.BindPushConstant(pushConstant);
				command.DispatchComputeIndirect(registry.GetBuffer(RGResource(ShadowSubmeshDispatches)), 0);
			};
		});

		renderGraph->AddPass(TargetQueueFamily::Compute, "ShadowMeshletDispatchBuildPass", [=, *this](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.ReadBuffer(RGResource(ShadowVisibleSubmeshCount), RenderGraphResourceAccess::StorageRead);

			build.WriteBuffer(RGResource(ShadowMeshletDispatches), RenderGraphResourceAccess::StorageWrite);

			return [=](RenderGraphRegistry& registry, RenderCommand& command) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::ShadowMeshletDispatchBuildPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("GPUBuildShadowMeshletDispatchPipeline");
				command.BindPipeline(pipeline);

				auto& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				auto& pushConstantData = s_ShadowCullPushConstants[Renderer::GetCurrentFrameIndex()];
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				command.BindPushConstant(pushConstant);
				command.DispatchCompute(1, 1, 1);
			};
		});

		renderGraph->AddPass(TargetQueueFamily::Compute, "ShadowMeshletCullPass", [=, *this](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.ReadExternalBuffer(RGResource(GPUSceneBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUObjectsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUMeshesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUMeshLODsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUSubmeshesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUMeshletsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUCullViewsBuffer), RenderGraphResourceAccess::StorageRead);

			build.ReadBuffer(RGResource(ShadowVisibleSubmeshes), RenderGraphResourceAccess::StorageRead);
			build.ReadBuffer(RGResource(ShadowVisibleSubmeshCount), RenderGraphResourceAccess::StorageRead);
			build.ReadBuffer(RGResource(ShadowMeshletDispatches), RenderGraphResourceAccess::IndirectRead);

			build.WriteBuffer(RGResource(ShadowVisibleDraws), RenderGraphResourceAccess::StorageWrite);
			build.WriteBuffer(RGResource(ShadowIndirectCommands), RenderGraphResourceAccess::StorageWrite);
			build.WriteBuffer(RGResource(ShadowDrawCounts), RenderGraphResourceAccess::StorageReadWrite);

			return [=](RenderGraphRegistry& registry, RenderCommand& command) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::ShadowMeshletCullPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("GPUCullShadowMeshletsPipeline");
				command.BindPipeline(pipeline);
				command.BindAllDescriptorSets();

				auto& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				auto& pushConstantData = s_ShadowCullPushConstants[Renderer::GetCurrentFrameIndex()];
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				command.BindPushConstant(pushConstant);
				command.DispatchComputeIndirect(registry.GetBuffer(RGResource(ShadowMeshletDispatches)), 0);
			};
		});

		renderGraph->AddPass(TargetQueueFamily::Graphics, "ShadowDrawPass", [=](RenderGraphBuilder& build) {
			build.SetViewportArea(m_ShadowMapSize, m_ShadowMapSize);

			build.SetInFlightMode(true);
			build.SetClearColor({ 1.0f, 1.0f, 1.0f, 1.0f });

			build.DeclareImage(RGResource(ShadowImages), {
				.Width = m_ShadowMapSize,
				.Height = m_ShadowMapSize,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
				.Layers = RenderDeviceSceneGlobalData::NUM_CASCADES,
				.Format = ImageFormat::R32G32_SFLOAT,
				.GenerateSampler = true
				}, RenderPassLoadStoreAttachments::ClearStore,
				RGResource(VSMDepth), {
					.Width = m_ShadowMapSize,
					.Height = m_ShadowMapSize,
					.ImageType = ImageType::Type2D,
					.ImageUsage = ImageUsage::AsDepthAttachment,
					.Layers = RenderDeviceSceneGlobalData::NUM_CASCADES,
					.Format = ImageFormat::D32_SFLOAT
				}, RenderPassLoadStoreAttachments::ClearStore
			);

			build.ReadExternalBuffer(RGResource(GPUSceneBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUObjectsBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUSubmeshesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUVerticesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUIndicesBuffer), RenderGraphResourceAccess::IndexRead);

			build.ReadBuffer(RGResource(ShadowVisibleDraws), RenderGraphResourceAccess::StorageRead);
			build.ReadBuffer(RGResource(ShadowIndirectCommands), RenderGraphResourceAccess::IndirectRead);
			build.ReadBuffer(RGResource(ShadowDrawCounts), RenderGraphResourceAccess::IndirectRead);

			build.BindRenderTarget(RGResource(ShadowImages), RGResource(VSMDepth));

			return [=](RenderGraphRegistry& registry, RenderCommand& draw) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::ShadowDrawPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("VSMPipeline");
				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.BindBuffers(registry.GetBuffer(RGResource(GPUIndicesBuffer)));

				struct LocalPushConstant {
					RenderDeviceBufferReference VisibleDraws;
				};

				auto& pushConstant = pipeline->GetPipelineConstants("PushConstants");

				uint64_t visibleDrawStride = static_cast<uint64_t>(commandCapacity) * sizeof(RenderDeviceVisibleDrawData);
				uint64_t indirectCommandStride = static_cast<uint64_t>(commandCapacity) * sizeof(VkDrawIndexedIndirectCommand);

				GlobalPushConstant<LocalPushConstant> pushConstantData{
					.Root = registry.GetBuffer(RGResource(GPUSceneBuffer))->GetDeviceAddress(),
					.Data = {
						.VisibleDraws = registry.GetBuffer(RGResource(ShadowVisibleDraws))->GetDeviceAddress(),
					}
				};

				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));
				draw.BindPushConstant(pushConstant);

				draw.DrawIndexedIndirectCount(registry.GetBuffer(RGResource(ShadowIndirectCommands)), 0,
					registry.GetBuffer(RGResource(ShadowDrawCounts)), 0, commandCapacity, sizeof(VkDrawIndexedIndirectCommand));
			};
		});

		enum class GaussianBlurDirection : uint8_t {
			Horizontal,
			Vertical
		};

		const auto ExecuteGaussianBlur = [=](RenderGraphRegistry& registry, RenderCommand& cmd, GaussianBlurDirection direction) {
			LUCY_PROFILE_NEW_EVENT("RendererPasses::ExecuteGaussianBlur");

			const auto& shadowImages = registry.GetImage(RGResource(ShadowImages));
			const auto& shadowImagesBlurred = registry.GetImage(RGResource(ShadowImagesBlurred));
			const auto& shadowImagesFinal = registry.GetImage(RGResource(ShadowImagesFinal));

			auto width = shadowImages->GetWidth();
			auto height = shadowImages->GetHeight();

			const auto& blurPipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>(direction == GaussianBlurDirection::Horizontal 
				? "VSMHorizontalBlurComputePipeline" : "VSMVerticalBlurComputePipeline");
			auto& pushConstant = blurPipeline->GetPipelineConstants("PushConstants");

			cmd.BindPipeline(blurPipeline);

			struct BlurData {
				int32_t BlurData[4];            // x, y, width, height
				uint32_t TextureIndicesData[4]; // x: input texture index, y: output texture index, z: sampler index, w: unused
			};

			if (direction == GaussianBlurDirection::Horizontal) {
				uint32_t inputIndex = cmd.BindImageHandleTo("TextureArrays2D_Float2", shadowImages);
				uint32_t outputIndex = cmd.BindImageHandleTo("StorageTextureArrays2D_Float2", shadowImagesBlurred);
				uint32_t samplerIndex = cmd.GetLinearRepeatSampler();

				BlurData data = {
					.BlurData = { 1, 0, width, height },
					.TextureIndicesData = { inputIndex, outputIndex, samplerIndex, 0 }
				};

				pushConstant.SetData(reinterpret_cast<uint8_t*>(&data), sizeof(data));
			} else {
				uint32_t inputIndex = cmd.BindImageHandleTo("TextureArrays2D_Float2", shadowImagesBlurred);
				uint32_t outputIndex = cmd.BindImageHandleTo("StorageTextureArrays2D_Float2", shadowImagesFinal);
				uint32_t samplerIndex = cmd.GetLinearRepeatSampler();

				BlurData data = {
					.BlurData = { 0, 1, width, height },
					.TextureIndicesData = { inputIndex, outputIndex, samplerIndex, 0 }
				};

				pushConstant.SetData(reinterpret_cast<uint8_t*>(&data), sizeof(data));
			}

			cmd.UpdateDescriptorSets();
			cmd.BindAllDescriptorSets();
			cmd.BindPushConstant(pushConstant);
			cmd.DispatchCompute((width + 7) / 8, (height + 7) / 8, RenderDeviceSceneGlobalData::NUM_CASCADES);
		};

		renderGraph->AddPass(TargetQueueFamily::Compute, "VSMHorizontalBlurCompute", [=, *this](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.DeclareImage(RGResource(ShadowImagesBlurred), {
				.Width = m_ShadowMapSize,
				.Height = m_ShadowMapSize,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
				.Layers = RenderDeviceSceneGlobalData::NUM_CASCADES,
				.Format = ImageFormat::R32G32_SFLOAT,
				.GenerateSampler = true,
			}, RenderPassLoadStoreAttachments::ClearDontCare);

			build.ReadImage(RGResource(ShadowImages), RenderGraphResourceAccess::ShaderSampledRead);
			build.WriteImage(RGResource(ShadowImagesBlurred), RenderGraphResourceAccess::StorageWrite);

			return std::bind(
				ExecuteGaussianBlur,
				std::placeholders::_1,
				std::placeholders::_2,
				GaussianBlurDirection::Horizontal
			);
		});

		renderGraph->AddPass(TargetQueueFamily::Compute, "VSMVerticalBlurCompute", [=, *this](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.DeclareImage(RGResource(ShadowImagesFinal), {
				.Width = m_ShadowMapSize,
				.Height = m_ShadowMapSize,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
				.Layers = RenderDeviceSceneGlobalData::NUM_CASCADES,
				.Format = ImageFormat::R32G32_SFLOAT,
				.GenerateSampler = true,
			}, RenderPassLoadStoreAttachments::ClearDontCare);
			
			build.ReadImage(RGResource(ShadowImagesBlurred), RenderGraphResourceAccess::ShaderSampledRead);
			build.WriteImage(RGResource(ShadowImagesFinal), RenderGraphResourceAccess::StorageWrite);

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
		static float cascadeSplits[RenderDeviceSceneGlobalData::NUM_CASCADES];

		float n = editorCamera.GetNearPlane() * ShadowCamera::GetNearPlaneFactor();
		float f = editorCamera.GetFarPlane() * ShadowCamera::GetFarPlaneFactor();
		float clipRange = f - n;

		float minZ = n;
		float maxZ = n + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		for (uint32_t i = 0; i < RenderDeviceSceneGlobalData::NUM_CASCADES; i++) {
			float iDivM = (i + 1) / (float)RenderDeviceSceneGlobalData::NUM_CASCADES;
			float C_iLog = minZ * std::pow(ratio, iDivM);
			float C_iUniform = minZ + range * iDivM;
			float C_i = lambda * (C_iLog - C_iUniform) + C_iUniform;
			cascadeSplits[i] = (C_i - n) / clipRange;
		}

		s_ShadowCameras.clear();
		s_ShadowCameras.reserve(RenderDeviceSceneGlobalData::NUM_CASCADES);

		for (uint32_t i = 0; i < RenderDeviceSceneGlobalData::NUM_CASCADES; i++)
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

		float radiusDistWS = std::ceil(glm::length(frustumCornersWS[0] - frustumCornersWS[6]) * 16.0f) / 16.0f;

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

	void ShadowCamera::CreateCullView(const Ref<RenderDevice>& device) {
		m_CullViewHandle = device->GetScene()->RegisterCullView({
			.Data = {
				INVALID_INDEX, INVALID_INDEX, 0,
				static_cast<uint32_t>(GPUCullViewFlags::EnableFrustumCulling) | static_cast<uint32_t>(GPUCullViewFlags::Orthographic)
			}
		});
	}

	void ShadowCamera::ResetSplit() {
		ShadowCamera::s_LastSplitDist = 0.0f;
	}

#pragma endregion ShadowPass

#pragma region DDGI

	DDGIPass::DDGIPass(uint32_t width, uint32_t height) 
		: m_Width(width), m_Height(height) {
		s_ProbeSphere = MeshFactory::CreateSphere(8, 12);
	}

	void DDGIPass::AddPass(const Ref<RenderGraph>& renderGraph) {
		constexpr uint32_t irradianceTexels = 8;
		constexpr uint32_t irradianceTileSize = irradianceTexels + 2;

		constexpr uint32_t depthTexels = 16;
		constexpr uint32_t depthTileSize = depthTexels + 2;

		const uint32_t probeColumns = m_ProbeCounts.x * m_ProbeCounts.z;
		const uint32_t probeRows = m_ProbeCounts.y;

		const uint32_t probeCount = probeColumns * probeRows;
		m_ProbeOrigin = -0.5f * glm::vec3(m_ProbeCounts - glm::vec3(1)) * m_ProbeSpacing;

		renderGraph->AddPass(TargetQueueFamily::Compute, "DDGITracePass", [probeColumns, probeRows, probeCount, this](RenderGraphBuilder& build) {
			build.SetInFlightMode(true);

			build.DeclareBuffer(RGResource(DDGIRayResults), {
				.DebugName = "DDGIRayResults",
				.Size = probeCount * s_RaysPerProbe * sizeof(RenderDeviceDDGIRayResult),
				.Usage = BufferUsage::Storage
			});

			build.DeclareImage(RGResource(DDGIIrradianceAtlas), {
				.Width = probeColumns * irradianceTileSize,
				.Height = probeRows * irradianceTileSize,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
				.Format = ImageFormat::R16G16B16A16_SFLOAT,
				.GenerateSampler = true,
				.ImGuiUsage = true,
			}, RenderPassLoadStoreAttachments::ClearStore);

			build.DeclareImage(RGResource(DDGIDepthAtlas), {
				.Width = probeColumns * depthTileSize,
				.Height = probeRows * depthTileSize,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
				.Format = ImageFormat::R16G16_SFLOAT,
				.GenerateSampler = true,
				.ImGuiUsage = true,
			}, RenderPassLoadStoreAttachments::ClearStore);

			build.WriteBuffer(RGResource(DDGIRayResults), RenderGraphResourceAccess::StorageWrite);

			return [=](RenderGraphRegistry& registry, RenderCommand& cmd) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::DDGITracePass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<RayTracingPipeline>("DDGITracePipeline");

				GlobalPushConstant<RenderDeviceDDGITraceData> pushConstantData {
					.Root = registry.GetBuffer(RGResource(GPUSceneBuffer))->GetDeviceAddress(),
					.Data = {
						.RayResults = registry.GetBuffer(RGResource(DDGIRayResults))->GetDeviceAddress(),
						.ProbeOriginAndMaxDistance = glm::vec4{ m_ProbeOrigin, 10000.0f },
						.ProbeSpacing = glm::vec4{ m_ProbeSpacing, 0.0f },
						.ProbeCountsAndRays = glm::uvec4{ m_ProbeCounts, s_RaysPerProbe }
					}
				};

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				cmd.BindPipeline(pipeline);
				cmd.BindPushConstant(pushConstant);
				bool tlasExists = cmd.UpdateDescriptorSets("u_TLAS");
				if (!tlasExists) {
					return;
				}
				cmd.BindAllDescriptorSets();
				cmd.TraceRays(s_RaysPerProbe, probeCount, 1);
			};
		});

		renderGraph->AddPass(TargetQueueFamily::Compute, "DDGIProbeUpdatePass", [](RenderGraphBuilder& build) {
			build.ReadBuffer(RGResource(DDGIRayResults), RenderGraphResourceAccess::StorageRead);

			build.WriteImage(RGResource(DDGIIrradianceAtlas), RenderGraphResourceAccess::StorageWrite);
			build.WriteImage(RGResource(DDGIDepthAtlas), RenderGraphResourceAccess::StorageWrite);

			return [=](RenderGraphRegistry& registry, RenderCommand& cmd) {

			};
		});

		renderGraph->AddPass(TargetQueueFamily::Graphics, "DDGIProbeDebugPass", [probeColumns, probeRows, probeCount, 
			width = m_Width, height = m_Height, origin = m_ProbeOrigin, spacing = m_ProbeSpacing, counts = m_ProbeCounts](RenderGraphBuilder& build) {
			build.SetViewportArea(width, height);
			build.SetInFlightMode(true);

			build.ReadExternalBuffer(RGResource(GPUSceneBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUVerticesBuffer), RenderGraphResourceAccess::StorageRead);
			build.ReadExternalBuffer(RGResource(GPUIndicesBuffer), RenderGraphResourceAccess::IndexRead);

			build.ReadImage(RGResource(GeometryImage), RenderGraphResourceAccess::ColorAttachmentWrite);
			build.ReadImage(RGResource(ObjectIDImage), RenderGraphResourceAccess::ColorAttachmentWrite);
			build.ReadImage(RGResource(GeometryDepthImage), RenderGraphResourceAccess::DepthAttachmentWrite);

			build.BindRenderTarget(RGResource(GeometryImage), RGResource(GeometryDepthImage));
			build.BindRenderTarget(RGResource(ObjectIDImage));

			struct DDGIProbeDebugData {
				RenderDeviceBufferReference RayResults;

				glm::vec4 ProbeOriginAndRadius;
				glm::vec4 ProbeSpacing;
				glm::uvec4 ProbeCountsAndRays;
			};

			return [=](RenderGraphRegistry& registry, RenderCommand& cmd) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::DDGIProbeDebugPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("DDGIProbeDebugPipeline");
				cmd.BindPipeline(pipeline);

				GlobalPushConstant<DDGIProbeDebugData> pushConstantData{
					.Root = registry.GetBuffer(RGResource(GPUSceneBuffer))->GetDeviceAddress(),
					.Data = {
						.RayResults = registry.GetBuffer(RGResource(DDGIRayResults))->GetDeviceAddress(),
						.ProbeOriginAndRadius = glm::vec4{ origin, 0.08f },
						.ProbeSpacing = glm::vec4{ spacing, 0.0f },
						.ProbeCountsAndRays = glm::uvec4{ counts, s_RaysPerProbe },
					}
				};

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));
				cmd.BindPushConstant(pushConstant);
				cmd.BindBuffers(registry.GetBuffer(RGResource(GPUIndicesBuffer)));

				cmd.DrawIndexed(s_ProbeSphere->GetIndicesSize(), probeCount, s_ProbeSphere->GetMyGlobalIndexOffset(), s_ProbeSphere->GetMyGlobalVertexOffset(), 0);
			};
		});
		
		renderGraph->AddPass(TargetQueueFamily::Compute, "DDGIDebugPass", [probeColumns, probeRows, probeCount, 
			width = m_Width, height = m_Height, origin = m_ProbeOrigin, spacing = m_ProbeSpacing, counts = m_ProbeCounts](RenderGraphBuilder& build) {
			struct DDGIDebugData {
				RenderDeviceBufferReference RayResults;
				uint32_t SceneColorTextureIndex;
				uint32_t SceneDepthTextureIndex;

				uint32_t ProbeCount;
				uint32_t MarkerRadiusPixels;
				uint32_t RaysPerProbe;
				uint32_t MaxRayDistance;
				uint32_t RayResultTextureIndex;

				glm::vec4 ProbeOrigin;
				glm::vec4 ProbeSpacing;

				glm::uvec4 ProbeCounts;
				glm::uvec4 OutputSize;
			};

			build.ReadBuffer(RGResource(DDGIRayResults), RenderGraphResourceAccess::StorageRead);

			build.DeclareImage(RGResource(DDGIRayDebugImage), {
				.Width = s_RaysPerProbe,
				.Height = probeCount,
				.ImageType = ImageType::Type2D,
				.ImageUsage = ImageUsage::AsColorStorageTransferAttachment,
				.Format = ImageFormat::R16G16B16A16_SFLOAT,
				.GenerateSampler = true,
				.ImGuiUsage = true,
			}, RenderPassLoadStoreAttachments::ClearStore);

			build.WriteImage(RGResource(DDGIRayDebugImage), RenderGraphResourceAccess::StorageWrite);

			return [=](RenderGraphRegistry& registry, RenderCommand& cmd) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::DDGIDebugPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("DDGIDebugPipeline");
				cmd.BindPipeline(pipeline);

				uint32_t rayResultIndex = cmd.BindImageHandleTo("StorageTextures2D", registry.GetImage(RGResource(DDGIRayDebugImage)));

				GlobalPushConstant<DDGIDebugData> pushConstantData{
					.Root = registry.GetBuffer(RGResource(GPUSceneBuffer))->GetDeviceAddress(),
					.Data = {
						.RayResults = registry.GetBuffer(RGResource(DDGIRayResults))->GetDeviceAddress(),
						.SceneColorTextureIndex = {},
						.SceneDepthTextureIndex = {},
						.ProbeCount = probeCount,
						.MarkerRadiusPixels = 2,
						.RaysPerProbe = s_RaysPerProbe,
						.MaxRayDistance = 10000,
						.RayResultTextureIndex = rayResultIndex,
						.ProbeOrigin = glm::vec4{ origin, 0.0f },
						.ProbeSpacing = glm::vec4{ spacing, 0.0f },
						.ProbeCounts = glm::uvec4{ counts, 0 },
						.OutputSize = glm::uvec4{ probeColumns * irradianceTileSize, probeRows * irradianceTileSize, 0, 0 }
					}
				};

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				cmd.BindPushConstant(pushConstant);
				cmd.BindAllDescriptorSets();
				cmd.DispatchCompute((s_RaysPerProbe + 7) / 8, (probeCount + 7) / 8, 1);
			};
		});
	}

#pragma endregion DDGI

#pragma region CubemapPass

	CubemapPass::CubemapPass(Ref<Scene> scene, uint32_t width, uint32_t height)
		: m_Scene(scene), m_Width(width), m_Height(height) {
	}

	void CubemapPass::AddPass(const Ref<RenderGraph>& renderGraph) {

		renderGraph->AddPass(TargetQueueFamily::Graphics, "CubemapPass", [*this](RenderGraphBuilder& build) {
			build.SetViewportArea(m_Width, m_Height);
			build.SetInFlightMode(true);

			build.ReadImage(RGResource(GeometryImage), RenderGraphResourceAccess::ColorAttachmentWrite);
			build.ReadImage(RGResource(ObjectIDImage), RenderGraphResourceAccess::ColorAttachmentWrite);
			build.ReadImage(RGResource(GeometryDepthImage), RenderGraphResourceAccess::DepthAttachmentWrite);

			build.BindRenderTarget(RGResource(GeometryImage), RGResource(GeometryDepthImage));
			build.BindRenderTarget(RGResource(ObjectIDImage));

			return [=](RenderGraphRegistry& registry, RenderCommand& draw) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::CubemapPass");
				
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("SkyboxPipeline");
				const auto& settings = Renderer::GetRendererSettings();

				bool imageBound = false;
				const Unique<Mesh>& cubeMesh = Renderer::GetEnvCubeMesh();

				draw.BindPipeline(pipeline);

				uint32_t index = INVALID_INDEX;

				m_Scene->ViewForEach<HDRCubemapComponent>([&imageBound, &index, &draw, &registry](const HDRCubemapComponent& hdrComponent) {
					if (!hdrComponent.IsPrimary || imageBound)
						return;
					index = draw.BindImageHandleTo("CubeTextures", registry.GetImage(RGResource(PrefilterImage)));
					imageBound = true;
				});

				if (!imageBound) {
					return;
				}

				struct LocalPushConstant {
					glm::vec3 Data; // x: environment map index, y: sampler index, z: mip level
				};

				GlobalPushConstant<LocalPushConstant> pushConstantData {
					registry.GetBuffer(RGResource(GPUSceneBuffer))->GetDeviceAddress(),
					{ .Data = { index, static_cast<float>(draw.GetLinearRepeatSampler().Index), settings.EnvironmentLOD}}
				};

				auto& pushConstant = pipeline->GetPipelineConstants("PushConstants");
				pushConstant.SetData(reinterpret_cast<uint8_t*>(&pushConstantData), sizeof(pushConstantData));

				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.BindPushConstant(pushConstant);
				draw.DrawMesh(cubeMesh);
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

			return [=](RenderGraphRegistry& registry, RenderCommand& draw) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::HDRImageToLayeredImage");
				static const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

				struct LocalPushConstant {
					glm::mat4 CaptureProjection;
					glm::uvec2 Data;
				};

				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("HDRImageToLayeredImageConvertPipeline");

				const auto& cubeMesh = Renderer::GetEnvCubeMesh();
				const uint32_t cubeMeshIndexCount = Renderer::GetEnvCubeMeshIndexCount();
				draw.BindPipeline(pipeline);

				uint32_t originalHDRImageIndex = draw.BindImageHandleTo("Textures2D", registry.GetImage(RGResource(OriginalHDRImage)));

				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.BindBuffers(registry.GetBuffer(RGResource(GPUIndicesBuffer)));

				GlobalPushConstant<LocalPushConstant> localPushConstant{
					.Root = registry.GetBuffer(RGResource(GPUSceneBuffer))->GetDeviceAddress(),
					.Data = {
						.CaptureProjection = captureProjection, 
						.Data = { originalHDRImageIndex, draw.GetLinearRepeatSampler() } 
					}
				};

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");

				ByteBuffer pushConstantData;
				pushConstantData.SetData((uint8_t*)&localPushConstant, sizeof(localPushConstant));
				pushConstant.SetData(pushConstantData);

				draw.BindPushConstant(pushConstant);
				draw.DrawIndexed(cubeMeshIndexCount, 1, 0, 0, 0);
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

			return [=](RenderGraphRegistry& registry, RenderCommand& draw) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::IrradiancePass");
				static const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
				
				static constexpr const uint32_t layerCount = 6;
				static constexpr const uint32_t workGroupSize = 8;

				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("IrradianceComputePipeline");

				const auto& cubeImage = registry.GetImage(RGResource(HDRLayeredImage));
				const auto& irradianceImage = registry.GetImage(RGResource(IrradianceImage));

				draw.BindPipeline(pipeline);

				PipelineConstant& pushConstant = pipeline->GetPipelineConstants("PushConstants");

				uint32_t layeredImageIndex = draw.BindImageHandleTo("CubeTextures", cubeImage);
				uint32_t irradianceImageIndex = draw.BindImageHandleTo("StorageTextureArrays2D", irradianceImage);

				struct LocalPushConstant {
					glm::mat4 ProjMatrix;
					glm::vec4 Data; // x: size, y: sampler index, z: environment map index, w: environment irradiance compute index
				};

				GlobalPushConstant<LocalPushConstant> pushConstantData{
					.Root = registry.GetBuffer(RGResource(GPUSceneBuffer))->GetDeviceAddress(),
					.Data = {
						.ProjMatrix = captureProjection,
						.Data = { m_Size, static_cast<float>(draw.GetLinearClampSampler().Index), layeredImageIndex, irradianceImageIndex }
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

			return [=](RenderGraphRegistry& registry, RenderCommand& draw) {
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<GraphicsPipeline>("IrradiancePipeline");
				const auto& mesh = Renderer::GetEnvCubeMesh();

				const auto& environmentMap = registry.GetImage(RGResource(HDRLayeredImage));

				pipeline->BindGlobalImageHandleTo("u_EnvironmentMap", environmentMap);

				draw.BindPipeline(pipeline);
				draw.UpdateDescriptorSets();
				draw.BindAllDescriptorSets();
				draw.BindBuffers(mesh);
				draw.DrawIndexed(36, 1, 0, 0, 0);
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

			return [=](RenderGraphRegistry& registry, RenderCommand& cmd) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::PrefilterPass");
				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("PrefilterComputePipeline");

				static constexpr uint32_t workGroupSize = 8;
				
				struct LocalPushConstant {
					glm::vec4 PrefilterParams; // x: sizeX, y: sizeY, z: roughness, w: mip
					glm::uvec3 TextureData;    // x: environment map index, y: prefilter map index, z: sampler index
				};

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
						.Root = registry.GetBuffer(RGResource(GPUSceneBuffer))->GetDeviceAddress(),
						.Data = {
							.PrefilterParams = glm::vec4(mipSize, mipSize, mip / float(MAX_MIP_LEVELS - 1), mip),
							.TextureData = { layeredImageIndex, prefilterImageIndices[mip], cmd.GetLinearClampSampler() }
						}
					};

					pushConstant.SetData((uint8_t*)&params, sizeof(params));

					const uint32_t groupsX = (mipSize + workGroupSize - 1) / workGroupSize;
					const uint32_t groupsY = (mipSize + workGroupSize - 1) / workGroupSize;

					cmd.BindPushConstant(pushConstant);
					cmd.DispatchCompute(groupsX, groupsY, 6);
				}
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

			return [=](RenderGraphRegistry& registry, RenderCommand& draw) {
				LUCY_PROFILE_NEW_EVENT("RendererPasses::BRDFLutPass");
				static constexpr uint32_t workGroupSize = 8;

				const auto& pipeline = Renderer::GetPipelineManager()->GetAs<ComputePipeline>("BRDFLutComputePipeline");
				const auto& brdfLutImage = registry.GetImage(RGResource(BRDFLutImage));

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