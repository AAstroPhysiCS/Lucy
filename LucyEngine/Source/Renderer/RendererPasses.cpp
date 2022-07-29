#include "lypch.h"
#include "RendererModule.h"

#include "Renderer/Renderer.h"

namespace Lucy {
	
	/* --- Individual Passes --- All passes should be in here */

	void ForwardRenderMeshes(VkCommandBuffer commandBuffer, const Ref<Pipeline>& pipeline, StaticMeshRenderCommand* staticMeshRenderCommand) {
		const Ref<Mesh>& staticMesh = staticMeshRenderCommand->Mesh;
		const glm::mat4& entityTransform = staticMeshRenderCommand->EntityTransform;

		const std::vector<Ref<Material>>& materials = staticMesh->GetMaterials();
		std::vector<Submesh>& submeshes = staticMesh->GetSubmeshes();

		Renderer::BindPipeline(commandBuffer, pipeline);
		Renderer::BindAllDescriptorSets(commandBuffer, pipeline);
		Renderer::BindBuffers(commandBuffer, staticMesh);

		PushConstant& meshPushConstant = pipeline->GetPushConstants("LocalPushConstant");

		for (uint32_t i = 0; i < submeshes.size(); i++) {
			Submesh& submesh = submeshes[i];
			const Ref<Material>& material = materials[submesh.MaterialIndex];

			PushConstantData pushConstantData;
			pushConstantData.FinalTransform = entityTransform * submesh.Transform;
			pushConstantData.MaterialID = material->GetID();
			meshPushConstant.SetData((uint8_t*)&pushConstantData, sizeof(pushConstantData));

			Renderer::BindPushConstant(commandBuffer, pipeline, meshPushConstant);
			Renderer::DrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndexCount, submesh.BaseVertexCount, 0);
		}
	}

	void GeometryPass(void* commandBuffer, Ref<Pipeline> geometryPipeline, RenderCommand* staticMeshRenderCommand) {
		ForwardRenderMeshes((VkCommandBuffer)commandBuffer, geometryPipeline, (StaticMeshRenderCommand*)staticMeshRenderCommand);
	}

	void IDPass(void* commandBuffer, Ref<Pipeline> idPipeline, RenderCommand* staticMeshRenderCommand) {
		ForwardRenderMeshes((VkCommandBuffer)commandBuffer, idPipeline, (StaticMeshRenderCommand*)staticMeshRenderCommand);
	}
}