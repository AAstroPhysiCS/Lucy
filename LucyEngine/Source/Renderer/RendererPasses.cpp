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
			pushConstantData.MaterialID = material->GetID();
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

		Renderer::DrawIndexed(commandBuffer, cubeMesh->GetIndexBuffer()->GetSize(), 1, 0, 0, 0);
	}

	struct CubePushConstantData {
		glm::mat4 Proj;
	};

	void PrepareEnvironmentalCube(void* commandBuffer, Ref<ContextPipeline> pipeline, RenderCommand* command) {
		CubeRenderCommand* environmentRenderCommand = (CubeRenderCommand*)command;

		static glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

		auto imageBuffer = pipeline->GetUniformBuffers<VulkanUniformImageBuffer>("u_EquirectangularMap");
		imageBuffer->BindImage(environmentRenderCommand->ImageView, environmentRenderCommand->Layout, environmentRenderCommand->Sampler);

		const Ref<Mesh>& cubeMesh = environmentRenderCommand->CubeMesh;

		Renderer::UpdateDescriptorSets(pipeline);
		Renderer::BindPipeline(commandBuffer, pipeline);
		Renderer::BindAllDescriptorSets(commandBuffer, pipeline);
		Renderer::BindBuffers(commandBuffer, cubeMesh);

		VulkanPushConstant& pushConstant = pipeline->GetPushConstants("LucyCameraPushConstants");

		CubePushConstantData pushConstantData;
		pushConstantData.Proj = captureProjection;
		pushConstant.SetData((uint8_t*)&pushConstantData, sizeof(CubePushConstantData));

		for (uint32_t i = 0; i < 6; i++) {
			Renderer::BindPushConstant(commandBuffer, pipeline, pushConstant);
			Renderer::DrawIndexed(commandBuffer, cubeMesh->GetIndexBuffer()->GetSize(), 1, 0, 0, 0);
		}
	}

	void ComputeIrradiancePass(void* commandBuffer, Ref<ContextPipeline> pipeline, RenderCommand* command) {
		//static Fence fence;

		ComputeDispatchCommand* dispatchCommand = (ComputeDispatchCommand*)command;
		Renderer::UpdateDescriptorSets(pipeline);

		Renderer::BindPipeline(commandBuffer, pipeline);
		Renderer::BindAllDescriptorSets(commandBuffer, pipeline);
		Renderer::DispatchCompute(commandBuffer, pipeline.As<ComputePipeline>(), dispatchCommand->GetGroupCountX(), dispatchCommand->GetGroupCountY(), dispatchCommand->GetGroupCountZ());

		//TODO: Destroy after computing (sync?)
		//Renderer::EnqueueResourceFree([pipeline]() {
		//	pipeline->Destroy();
		//}, &fence);
	}
}