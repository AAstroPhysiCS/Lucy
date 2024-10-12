#include "lypch.h"
#include "RenderCommand.h"

#include "Renderer/Device/RenderDevice.h"

#include "Renderer/Commands/CommandPool.h"
#include "Renderer/Mesh.h"

#include "Renderer/Pipeline/GraphicsPipeline.h"
#include "Renderer/Pipeline/ComputePipeline.h"

namespace Lucy {

	RenderCommand::RenderCommand(const std::string& nameOfDraw, const Ref<RenderDevice>& renderDevice, const Ref<CommandPool>& primaryCmdPool)
		: m_DebugName(nameOfDraw), m_RenderDevice(renderDevice), m_PrimaryCommandPool(primaryCmdPool) {
	}

	void RenderCommand::BeginSecondaryRenderCommand() {
		//TODO:
	}

	void RenderCommand::EndSecondaryRenderCommand() {
		//TODO:
	}

	void RenderCommand::BeginDebugMarker() {
		m_RenderDevice->BeginDebugMarker(m_PrimaryCommandPool, m_DebugName.c_str());
	}

	void RenderCommand::EndDebugMarker() {
		m_RenderDevice->EndDebugMarker(m_PrimaryCommandPool);
	}

	void RenderCommand::BeginTimestamp() {
		m_BeginTimestampIndex = m_RenderDevice->BeginTimestamp(m_PrimaryCommandPool);
	}

	void RenderCommand::EndTimestamp() {
		m_EndTimestampIndex = m_RenderDevice->EndTimestamp(m_PrimaryCommandPool);
	}

	void RenderCommand::BeginPipelineStatistics() {
		//m_RenderDevice->BeginPipelineQuery(m_PrimaryCommandPool);
	}

	void RenderCommand::EndPipelineStatistics() {
		//m_RenderDevice->EndPipelineQuery(m_PrimaryCommandPool);

		//if (auto pipeline = m_BoundedGraphicsPipeline; pipeline)
		//	pipeline->Unbind(m_RenderDevice->GetQueryResults(RenderDeviceQueryType::Pipeline));
	}

	void RenderCommand::BindBuffers(Ref<Mesh> mesh) {
		LUCY_ASSERT(m_BoundedGraphicsPipeline, "BindBuffers failed, bounded pipeline is nullptr.");
		m_RenderDevice->BindBuffers(m_PrimaryCommandPool, mesh);
	}

	void RenderCommand::BindBuffers(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) {
		LUCY_ASSERT(m_BoundedGraphicsPipeline, "BindBuffers failed, bounded pipeline is nullptr.");
		m_RenderDevice->BindBuffers(m_PrimaryCommandPool, vertexBuffer, indexBuffer);
	}

	void RenderCommand::BindPushConstant(const VulkanPushConstant& pushConstant) {
		LUCY_ASSERT(m_BoundedGraphicsPipeline || m_BoundedComputePipeline, "BindPushConstant failed, bounded pipeline is nullptr.");
		if (m_BoundedGraphicsPipeline) {
			m_RenderDevice->BindPushConstant(m_PrimaryCommandPool, m_BoundedGraphicsPipeline, pushConstant);
			return;
		}
		m_RenderDevice->BindPushConstant(m_PrimaryCommandPool, m_BoundedComputePipeline, pushConstant);
	}

	void RenderCommand::BindPipeline(const Ref<GraphicsPipeline>& pipeline) {
		LUCY_ASSERT(pipeline, "BindPipeline failed, pipeline is nullptr.");
		m_RenderDevice->BindPipeline(m_PrimaryCommandPool, pipeline);
		BeginPipelineStatistics();
		m_Shader = pipeline->GetShader();
		m_BoundedGraphicsPipeline = pipeline;
	}

	void RenderCommand::BindPipeline(const Ref<ComputePipeline>& pipeline) {
		LUCY_ASSERT(pipeline, "BindPipeline failed, pipeline is nullptr.");
		m_RenderDevice->BindPipeline(m_PrimaryCommandPool, pipeline);
		BeginPipelineStatistics();
		m_Shader = pipeline->GetShader();
		m_BoundedComputePipeline = pipeline;
	}

	void RenderCommand::UpdateDescriptorSets() {
		if (m_BoundedGraphicsPipeline) {
			m_RenderDevice->UpdateDescriptorSets(m_BoundedGraphicsPipeline);
			return;
		}
		m_RenderDevice->UpdateDescriptorSets(m_BoundedComputePipeline);
	}

	void RenderCommand::BindAllDescriptorSets() {
		LUCY_ASSERT(m_BoundedGraphicsPipeline || m_BoundedComputePipeline, "BindAllDescriptorSets failed, bounded pipeline is nullptr.");
		if (m_BoundedGraphicsPipeline) {
			m_RenderDevice->BindAllDescriptorSets(m_PrimaryCommandPool, m_BoundedGraphicsPipeline);
			return;
		}
		m_RenderDevice->BindAllDescriptorSets(m_PrimaryCommandPool, m_BoundedComputePipeline);
	}

	void RenderCommand::BindDescriptorSet(uint32_t setIndex) {
		LUCY_ASSERT(m_BoundedGraphicsPipeline || m_BoundedComputePipeline, "BindDescriptorSet failed, bounded pipeline is nullptr.");
		if (m_BoundedGraphicsPipeline) {
			m_RenderDevice->BindDescriptorSet(m_PrimaryCommandPool, m_BoundedGraphicsPipeline, setIndex);
			return;
		}
		m_RenderDevice->BindDescriptorSet(m_PrimaryCommandPool, m_BoundedComputePipeline, setIndex);
	}

	void RenderCommand::DrawIndexedMesh(Ref<Mesh> mesh, const glm::mat4& meshTransform) {
		LUCY_ASSERT(m_BoundedGraphicsPipeline, "DrawIndexedMeshWithMaterial failed, bounded pipeline is nullptr.");
		LUCY_ASSERT(m_Shader, "DrawIndexedMeshWithMaterial failed, shader is nullptr.");

		BindBuffers(mesh);

		VulkanPushConstant& meshPushConstant = m_Shader->GetPushConstants("LocalPushConstant");

		const auto& submeshes = mesh->GetSubmeshes();

		for (uint32_t i = 0; i < submeshes.size(); i++) {
			const Submesh& submesh = submeshes[i];

			const glm::mat4& finalTransform = meshTransform * submesh.Transform;

			ByteBuffer pushConstantData;
			pushConstantData.Append((uint8_t*)&finalTransform, sizeof(finalTransform));

			meshPushConstant.SetData(pushConstantData);

			BindPushConstant(meshPushConstant);
			DrawIndexed(submesh.IndexCount, 1, submesh.BaseIndexCount, submesh.BaseVertexCount, 0);
		}
	}

	void RenderCommand::DrawIndexedMeshWithMaterial(Ref<Mesh> mesh, const glm::mat4& meshTransform) {
		LUCY_ASSERT(m_BoundedGraphicsPipeline, "DrawIndexedMeshWithMaterial failed, bounded pipeline is nullptr.");
		LUCY_ASSERT(m_Shader, "DrawIndexedMeshWithMaterial failed, shader is nullptr.");
		
		BindBuffers(mesh);

		VulkanPushConstant& meshPushConstant = m_Shader->GetPushConstants("LocalPushConstant");

		const auto& submeshes = mesh->GetSubmeshes();

		for (uint32_t i = 0; i < submeshes.size(); i++) {
			const Submesh& submesh = submeshes[i];
			MaterialID materialID = submesh.MaterialID;

			const glm::mat4& finalTransform = meshTransform * submesh.Transform;

			ByteBuffer pushConstantData;
			pushConstantData.Append((uint8_t*)&finalTransform, sizeof(finalTransform));
			pushConstantData.Append((uint8_t*)&materialID, sizeof(MaterialID));

			meshPushConstant.SetData(pushConstantData);

			BindPushConstant(meshPushConstant);
			DrawIndexed(submesh.IndexCount, 1, submesh.BaseIndexCount, submesh.BaseVertexCount, 0);
		}
	}

	void RenderCommand::DrawMesh(Ref<Mesh> mesh) {
		LUCY_ASSERT(m_BoundedGraphicsPipeline, "DrawMesh failed, bounded pipeline is nullptr.");
		BindBuffers(mesh);
		m_RenderDevice->DrawIndexed(m_PrimaryCommandPool, (uint32_t)m_RenderDevice->AccessResource<IndexBuffer>(mesh->GetIndexBufferHandle())->GetSize(), 1, 0, 0, 0);
	}

	void RenderCommand::DrawMeshWithPushConstant(Ref<Mesh> mesh) {
		LUCY_ASSERT(m_BoundedGraphicsPipeline, "DrawMeshWithPushConstant failed, bounded pipeline is nullptr.");
	}

	void RenderCommand::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
		m_RenderDevice->DrawIndexed(m_PrimaryCommandPool, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void RenderCommand::DispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
		LUCY_ASSERT(m_BoundedComputePipeline, "DispatchCompute failed, bounded pipeline is nullptr.");
		m_RenderDevice->DispatchCompute(m_PrimaryCommandPool, m_BoundedComputePipeline, groupCountX, groupCountY, groupCountZ);
	}

	constexpr void RenderCommand::DisableBackCulling(bool backCullingDisable) {
		m_DynamicRasterizationConfig.DisableBackCulling = backCullingDisable;
	}

	constexpr void RenderCommand::SetCullingMode(CullingMode cullingMode) {
		m_DynamicRasterizationConfig.CullingMode = cullingMode;
	}

	constexpr void RenderCommand::SetLineWidth(float lineWidth) {
		m_DynamicRasterizationConfig.LineWidth = lineWidth;
	}

	constexpr void RenderCommand::SetPolygonMode(PolygonMode polygonMode) {
		m_DynamicRasterizationConfig.PolygonMode = polygonMode;
	}

	constexpr void RenderCommand::SetClearColor(ClearColor clearColor) {
		m_DynamicClearColor = clearColor;
	}

	constexpr void RenderCommand::SetDepthWriteEnable(bool depthWriteEnable) {
		m_DynamicDepthConfig.DepthWriteEnable = depthWriteEnable;
	}

	constexpr void RenderCommand::SetDepthTestEnable(bool depthTestEnable) {
		m_DynamicDepthConfig.DepthTestEnable = depthTestEnable;
	}

	constexpr void RenderCommand::SetDepthCompareOp(DepthCompareOp depthCompareOp) {
		m_DynamicDepthConfig.DepthCompareOp = depthCompareOp;
	}

	constexpr void RenderCommand::SetStencilTestEnable(bool stencilTestEnable) {
		m_DynamicDepthConfig.StencilTestEnable = stencilTestEnable;
	}
}
