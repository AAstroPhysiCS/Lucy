#include "lypch.h"
#include "RendererModule.h"

#include "Renderer.h"
#include "Memory/Buffer/Vulkan/VulkanUniformBuffer.h"
#include "Memory/Buffer/Vulkan/VulkanVertexBuffer.h"

namespace Lucy {

	/* --- Individual Passes --- All passes should be in here */

	struct MeshPushConstantData {
		glm::mat4 FinalTransform;
		float MaterialID;
	};

	void ForwardRenderMeshes(VkCommandBuffer commandBuffer, const Ref<GraphicsPipeline>& pipeline, StaticMeshRenderCommand* staticMeshRenderCommand, const char* debugEventName) {
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

	void GeometryPass(void* commandBuffer, Ref<GraphicsPipeline> geometryPipeline, RenderCommand* staticMeshRenderCommand) {
		ForwardRenderMeshes((VkCommandBuffer)commandBuffer, geometryPipeline, (StaticMeshRenderCommand*)staticMeshRenderCommand, "Geometry Pass");
	}

	void IDPass(void* commandBuffer, Ref<GraphicsPipeline> idPipeline, RenderCommand* staticMeshRenderCommand) {
		ForwardRenderMeshes((VkCommandBuffer)commandBuffer, idPipeline, (StaticMeshRenderCommand*)staticMeshRenderCommand, "ID Pass");
	}

	struct CubePushConstantData {
		glm::mat4 Proj;
		int32_t ColorAttachmentOutputIndex;
	};

	void RenderEnvironmentalCube(void* commandBuffer, Ref<GraphicsPipeline> pipeline, RenderCommand* command) {
		InternalRenderCommand* environmentRenderCommand = (InternalRenderCommand*)command;

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

		for (uint32_t i = 0; i < 6; i++) {
			pushConstantData.ColorAttachmentOutputIndex = i;

			pushConstant.SetData((uint8_t*)&pushConstantData, sizeof(CubePushConstantData));

			Renderer::BindPushConstant(commandBuffer, pipeline, pushConstant);
			Renderer::DrawIndexed(commandBuffer, cubeMesh->GetIndexBuffer()->GetSize(), 1, 0, 0, 0);
		}
	}
}