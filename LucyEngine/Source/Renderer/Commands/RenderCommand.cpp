#include "lypch.h"
#include "RenderCommand.h"

#include "Renderer/Device/RenderDevice.h"

#include "Renderer/Commands/CommandPool.h"
#include "Renderer/Mesh.h"

#include "Renderer/Pipeline/GraphicsPipeline.h"
#include "Renderer/Pipeline/ComputePipeline.h"

#include "Renderer/Image/VulkanImage.h"

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
		m_BeginTimestampIndex = m_RenderDevice->RTBeginTimestamp(m_PrimaryCommandPool);
	}

	void RenderCommand::EndTimestamp() {
		m_EndTimestampIndex = m_RenderDevice->RTEndTimestamp(m_PrimaryCommandPool);
	}

	void RenderCommand::BeginPipelineStatistics() {
		m_RenderDevice->RTBeginPipelineQuery(m_PrimaryCommandPool);
	}

	void RenderCommand::EndPipelineStatistics() {
		if (!m_BoundedGraphicsPipeline)
			return;
		m_RenderDevice->RTEndPipelineQuery(m_PrimaryCommandPool);
		m_BoundedGraphicsPipeline->Unbind(m_RenderDevice->GetQueryResults(RenderDeviceQueryType::Pipeline));
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
		m_Shader = pipeline->GetShader();
		m_BoundedGraphicsPipeline = pipeline;
		BeginPipelineStatistics();
	}

	void RenderCommand::BindPipeline(const Ref<ComputePipeline>& pipeline) {
		LUCY_ASSERT(pipeline, "BindPipeline failed, pipeline is nullptr.");
		m_RenderDevice->BindPipeline(m_PrimaryCommandPool, pipeline);
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

	void RenderCommand::SetImageLayout(Ref<Image> image, uint32_t newLayout, uint32_t baseMipLevel, uint32_t baseArrayLayer, uint32_t levelCount, uint32_t layerCount) {
		if (Renderer::GetRenderArchitecture() != RenderArchitecture::Vulkan)
			return;
		const auto& vulkanImage = image->As<VulkanImage>();
		vulkanImage->SetLayout((VkCommandBuffer)m_PrimaryCommandPool->GetCurrentFrameCommandBuffer(),
			(VkImageLayout)newLayout, baseMipLevel, baseArrayLayer, levelCount, layerCount);
	}

	void RenderCommand::CopyImageToImage(Ref<Image> srcImage, Ref<Image> destImage, const std::vector<VkImageCopy>& regions) {
		if (Renderer::GetRenderArchitecture() != RenderArchitecture::Vulkan)
			return;
		const auto& srcVulkanImage = srcImage->As<VulkanImage>();
		const auto& destVulkanImage = destImage->As<VulkanImage>();
		srcVulkanImage->CopyImageToImage((VkCommandBuffer)m_PrimaryCommandPool->GetCurrentFrameCommandBuffer(), destVulkanImage, regions);
	}

	void RenderCommand::CopyBufferToImage(Ref<ByteBuffer> srcBuffer, Ref<Image> destImage) {
		//TODO:
		LUCY_ASSERT(false);
	}

	void RenderCommand::CopyImageToBuffer(Ref<Image> srcImage, Ref<ByteBuffer> destBuffer) {
		//TODO:
		LUCY_ASSERT(false);
	}

	void RenderCommand::DisableBackCulling(bool backCullingDisable) {
		m_DynamicRasterizationConfig.DisableBackCulling = backCullingDisable;
	}

	void RenderCommand::SetCullingMode(CullingMode cullingMode) {
		m_DynamicRasterizationConfig.CullingMode = cullingMode;
	}

	void RenderCommand::SetLineWidth(float lineWidth) {
		m_DynamicRasterizationConfig.LineWidth = lineWidth;
	}

	void RenderCommand::SetPolygonMode(PolygonMode polygonMode) {
		m_DynamicRasterizationConfig.PolygonMode = polygonMode;
	}

	void RenderCommand::SetClearColor(ClearColor clearColor) {
		m_DynamicClearColor = clearColor;
	}

	void RenderCommand::SetDepthWriteEnable(bool depthWriteEnable) {
		m_DynamicDepthConfig.DepthWriteEnable = depthWriteEnable;
	}

	void RenderCommand::SetDepthTestEnable(bool depthTestEnable) {
		m_DynamicDepthConfig.DepthTestEnable = depthTestEnable;
	}

	void RenderCommand::SetDepthCompareOp(DepthCompareOp depthCompareOp) {
		m_DynamicDepthConfig.DepthCompareOp = depthCompareOp;
	}

	void RenderCommand::SetStencilTestEnable(bool stencilTestEnable) {
		m_DynamicDepthConfig.StencilTestEnable = stencilTestEnable;
	}
}
