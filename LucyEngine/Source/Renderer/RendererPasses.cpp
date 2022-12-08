#include "lypch.h"
#include "RendererModule.h"

#include "Renderer.h"
#include "Memory/Buffer/Vulkan/VulkanUniformBuffer.h"
#include "Memory/Buffer/Vulkan/VulkanVertexBuffer.h"

#include "Renderer/Context/ContextPipeline.h"
#include "Renderer/Context/ComputePipeline.h"

#include "Synchronization/VulkanSyncItems.h"

namespace Lucy {

	/* --- Individual Passes --- All passes should be in here */

	struct MeshPushConstantData {
		glm::mat4 FinalTransform;
		float MaterialID;
	};

	void ForwardRenderMeshes(VkCommandBuffer commandBuffer, const Ref<ContextPipeline>& pipeline, StaticMeshRenderCommand* staticMeshRenderCommand, const char* debugEventName) {
		const Ref<Mesh>& staticMesh = staticMeshRenderCommand->Mesh;
		const glm::mat4& entityTransform = staticMeshRenderCommand->EntityTransform;

		const std::vector<Ref<Material>>& materials = staticMesh->GetMaterials();
		std::vector<Submesh>& submeshes = staticMesh->GetSubmeshes();

		Renderer::BindPipeline(commandBuffer, pipeline);
		Renderer::BindAllDescriptorSets(commandBuffer, pipeline);
		Renderer::BindBuffers(commandBuffer, staticMesh);

		VulkanPushConstant& meshPushConstant = pipeline->GetPushConstants("LocalPushConstant");

		for (uint32_t i = 0; i < submeshes.size(); i++) {
			Submesh& submesh = submeshes[i];
			const Ref<Material>& material = materials[submesh.MaterialIndex];

			MeshPushConstantData pushConstantData;
			pushConstantData.FinalTransform = entityTransform * submesh.Transform;
			pushConstantData.MaterialID = (float)material->GetID();
			meshPushConstant.SetData((uint8_t*)&pushConstantData, sizeof(pushConstantData));

			Renderer::BindPushConstant(commandBuffer, pipeline, meshPushConstant);
			Renderer::DrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndexCount, submesh.BaseVertexCount, 0);
		}
	}

	void GeometryPass(void* commandBuffer, Ref<ContextPipeline> geometryPipeline, RenderCommand* staticMeshRenderCommand) {
		ForwardRenderMeshes((VkCommandBuffer)commandBuffer, geometryPipeline, (StaticMeshRenderCommand*)staticMeshRenderCommand, "Geometry Pass");
	}

	void IDPass(void* commandBuffer, Ref<ContextPipeline> idPipeline, RenderCommand* staticMeshRenderCommand) {
		ForwardRenderMeshes((VkCommandBuffer)commandBuffer, idPipeline, (StaticMeshRenderCommand*)staticMeshRenderCommand, "ID Pass");
	}

	void CubemapPass(void* commandBuffer, Ref<ContextPipeline> cubemapPipeline, RenderCommand* cubemapRenderCommand) {
		CubeRenderCommand* environmentRenderCommand = (CubeRenderCommand*)cubemapRenderCommand;
		const Ref<Mesh>& cubeMesh = environmentRenderCommand->CubeMesh;

		Renderer::BindPipeline(commandBuffer, cubemapPipeline);
		Renderer::BindAllDescriptorSets(commandBuffer, cubemapPipeline);
		Renderer::BindBuffers(commandBuffer, cubeMesh);

		Renderer::DrawIndexed(commandBuffer, (uint32_t)cubeMesh->GetIndexBuffer()->GetSize(), 1, 0, 0, 0);
	}

	struct CubePushConstantData {
		glm::mat4 Proj;
	};

	void PrepareEnvironmentalCube(void* commandBuffer, Ref<ContextPipeline> pipeline, RenderCommand* command) {
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

		for (uint32_t i = 0; i < 6; i++) {
			Renderer::BindPushConstant(commandBuffer, pipeline, pushConstant);
			Renderer::DrawIndexed(commandBuffer, (uint32_t)cubeMesh->GetIndexBuffer()->GetSize(), 1, 0, 0, 0);
		}
	}

#if USE_COMPUTE_FOR_CUBEMAP_GEN
	void ComputeIrradiance(void* commandBuffer, Ref<ContextPipeline> pipeline, RenderCommand* command) {
		//static Fence fence;
		ComputeDispatchCommand* dispatchCommand = (ComputeDispatchCommand*)command;

		Renderer::BindPipeline(commandBuffer, pipeline);
		Renderer::BindAllDescriptorSets(commandBuffer, pipeline);
		Renderer::DispatchCompute(commandBuffer, pipeline.As<ComputePipeline>(), dispatchCommand->GetGroupCountX(), dispatchCommand->GetGroupCountY(), dispatchCommand->GetGroupCountZ());

		//TODO: Destroy after computing (sync?)
		//Renderer::EnqueueResourceFree([pipeline]() {
		//	pipeline->Destroy();
		//}, &fence);
	}

	void ComputePrefilter(void* commandBuffer, Ref<ContextPipeline> pipeline, RenderCommand* command) {
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
			Renderer::DispatchCompute(commandBuffer, pipeline.As<ComputePipeline>(), dispatchCommand->GetGroupCountX(), dispatchCommand->GetGroupCountY(), dispatchCommand->GetGroupCountZ());
		}
		*/
	}
#else
	void ComputeIrradiance(void* commandBuffer, Ref<ContextPipeline> pipeline, RenderCommand* command) {
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
		Renderer::DrawIndexed(commandBuffer, cubeMesh->GetIndexBuffer()->GetSize(), 1, 0, 0, 0);
	}

	void ComputePrefilter(void* commandBuffer, Ref<ContextPipeline> pipeline, RenderCommand* command) {
		Renderer::BindPipeline(commandBuffer, pipeline);
		Renderer::BindAllDescriptorSets(commandBuffer, pipeline);
		//TODO:
	}
#endif
}