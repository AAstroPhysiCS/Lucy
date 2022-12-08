#include "lypch.h"
#include "VulkanRenderDevice.h"
#include "Renderer/Context/VulkanContextDevice.h"

#include "Renderer/Context/VulkanGraphicsPipeline.h"
#include "Renderer/Context/VulkanComputePipeline.h"

#include "Renderer/Descriptors/VulkanDescriptorSet.h"

#include "Renderer/Memory/Buffer/Vulkan/VulkanVertexBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanIndexBuffer.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

#include "Renderer/Commands/VulkanCommandQueue.h"

#include "Renderer/Renderer.h"

#include "../Mesh.h"

namespace Lucy {

	void VulkanRenderDevice::Init() {
		RenderDevice::Init();

		VkQueryPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		createInfo.queryCount = s_QueryCount;
		createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;

		m_QueryPools.resize(Renderer::GetMaxFramesInFlight());
		for (uint32_t i = 0; i < Renderer::GetMaxFramesInFlight(); i++)
			LUCY_VK_ASSERT(vkCreateQueryPool(VulkanContextDevice::Get().GetLogicalDevice(), &createInfo, nullptr, &m_QueryPools[i]));
	}

	void VulkanRenderDevice::BindBuffers(void* commandBufferHandle, Ref<Mesh> mesh) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindBuffers");

		VulkanVertexBindInfo vertexInfo;
		vertexInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		mesh->GetVertexBuffer().As<VulkanVertexBuffer>()->Bind(vertexInfo);

		VulkanIndexBindInfo indexInfo;
		indexInfo.CommandBuffer = vertexInfo.CommandBuffer;
		mesh->GetIndexBuffer().As<VulkanIndexBuffer>()->Bind(indexInfo);
	}

	void VulkanRenderDevice::BindBuffers(void* commandBufferHandle, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindBuffers");

		VulkanVertexBindInfo vertexInfo;
		vertexInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		vertexBuffer.As<VulkanVertexBuffer>()->Bind(vertexInfo);

		VulkanIndexBindInfo indexInfo;
		indexInfo.CommandBuffer = vertexInfo.CommandBuffer;
		indexBuffer.As<VulkanIndexBuffer>()->Bind(indexInfo);
	}

	void VulkanRenderDevice::BindPushConstant(void* commandBufferHandle, Ref<ContextPipeline> pipeline, const VulkanPushConstant& pushConstant) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindPushConstant");
		switch (pipeline->GetType()) {
			case ContextPipelineType::Graphics:
				pushConstant.Bind((VkCommandBuffer)commandBufferHandle, pipeline.As<VulkanGraphicsPipeline>()->GetPipelineLayout());
				break;
			case ContextPipelineType::Compute:
				pushConstant.Bind((VkCommandBuffer)commandBufferHandle, pipeline.As<VulkanComputePipeline>()->GetPipelineLayout());
				break;
			default:
				LUCY_ASSERT(false);
		}
	}

	void VulkanRenderDevice::BindPipeline(void* commandBufferHandle, Ref<ContextPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindPipeline");
		pipeline->Bind(commandBufferHandle);
	}

	void VulkanRenderDevice::UpdateDescriptorSets(Ref<ContextPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::UpdateDescriptorSets");

		for (Ref<DescriptorSet> descriptorSet : pipeline->GetDescriptorSets()) {
			Ref<VulkanDescriptorSet> vulkanSet = descriptorSet.As<VulkanDescriptorSet>();
			vulkanSet->Update();
		}
	}

	//refactor bind, we dont need them technically. We can just call the e.g. bei pipeline
	//vulkanpipeline bind. We can have a bind there with specific stuff.
	//no need to have the parent class have a bind method (we cant generalize it like that)

	void VulkanRenderDevice::BindAllDescriptorSets(void* commandBufferHandle, Ref<ContextPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindAllDescriptorSets");

		VulkanDescriptorSetBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		switch (pipeline->GetType()) {
			case ContextPipelineType::Graphics:
				bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				bindInfo.PipelineLayout = pipeline.As<VulkanGraphicsPipeline>()->GetPipelineLayout();
				break;
			case ContextPipelineType::Compute:
				bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
				bindInfo.PipelineLayout = pipeline.As<VulkanComputePipeline>()->GetPipelineLayout();
				break;
			default:
				LUCY_ASSERT(false);
		}

		for (Ref<DescriptorSet> descriptorSet : pipeline->GetDescriptorSets()) {
			Ref<VulkanDescriptorSet> vulkanSet = descriptorSet.As<VulkanDescriptorSet>();
			vulkanSet->Bind(bindInfo);
		}
	}

	void VulkanRenderDevice::BindDescriptorSet(void* commandBufferHandle, Ref<ContextPipeline> pipeline, uint32_t setIndex) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindDescriptorSet");

		VulkanDescriptorSetBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		switch (pipeline->GetType()) {
			case ContextPipelineType::Graphics:
				bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				bindInfo.PipelineLayout = pipeline.As<VulkanGraphicsPipeline>()->GetPipelineLayout();
				break;
			case ContextPipelineType::Compute:
				bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
				bindInfo.PipelineLayout = pipeline.As<VulkanComputePipeline>()->GetPipelineLayout();
				break;
			default:
				LUCY_ASSERT(false);
		}

		for (Ref<DescriptorSet> descriptorSet : pipeline->GetDescriptorSets()) {
			if (descriptorSet->GetSetIndex() == setIndex) {
				Ref<VulkanDescriptorSet> vulkanSet = descriptorSet.As<VulkanDescriptorSet>();
				vulkanSet->Bind(bindInfo);
				break;
			}
		}
	}

	void VulkanRenderDevice::DrawIndexed(void* commandBufferHandle, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
		vkCmdDrawIndexed((VkCommandBuffer)commandBufferHandle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void VulkanRenderDevice::DispatchCompute(void* commandBufferHandle, Ref<ComputePipeline> computePipeline, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
		computePipeline.As<VulkanComputePipeline>()->Dispatch(commandBufferHandle, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanRenderDevice::BeginRenderPass(void* commandBufferHandle, Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BeginRenderPass");

		Ref<VulkanRenderPass> renderPass = pipeline->GetRenderPass();
		Ref<VulkanFrameBuffer> frameBuffer = pipeline->GetFrameBuffer();

		VulkanRenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		renderPassBeginInfo.Width = frameBuffer->GetWidth();
		renderPassBeginInfo.Height = frameBuffer->GetHeight();

		if (frameBuffer->IsInFlight())
			renderPassBeginInfo.VulkanFrameBuffer = frameBuffer->GetVulkanHandles()[Renderer::GetCurrentFrameIndex()];
		else
			renderPassBeginInfo.VulkanFrameBuffer = frameBuffer->GetVulkanHandles()[0];

		renderPass->Begin(renderPassBeginInfo);
	}

	void VulkanRenderDevice::EndRenderPass(Ref<GraphicsPipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::EndRenderPass");

		pipeline->GetRenderPass().As<VulkanRenderPass>()->End();
	}
		
	void VulkanRenderDevice::BeginTimestamp(void* commandBufferHandle) {
		vkCmdResetQueryPool((VkCommandBuffer)commandBufferHandle, m_QueryPools[Renderer::GetCurrentFrameIndex()], 0, s_QueryCount);
		
		uint32_t queryIndex = (uint32_t)m_QueryStack.size();
		vkCmdWriteTimestamp((VkCommandBuffer)commandBufferHandle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_QueryPools[Renderer::GetCurrentFrameIndex()], queryIndex);
		m_QueryStack.push(queryIndex);
	}

	double VulkanRenderDevice::GetTimestampResults() {
		static double localFrameTime = 0;

		uint32_t timestampData[2] = {};
		VkResult result = vkGetQueryPoolResults(VulkanContextDevice::Get().GetLogicalDevice(), m_QueryPools[Renderer::GetCurrentFrameIndex()], 0, 2,
												sizeof(timestampData), timestampData, sizeof(timestampData[0]), 0);
		if (result == VK_SUCCESS) {
			const float timestampPeriod = VulkanContextDevice::Get().GetTimestampPeriod();
			double beginFrameTime = timestampData[0] * timestampPeriod * 1e-6;
			double endFrameTime = timestampData[1] * timestampPeriod * 1e-6;

			localFrameTime = localFrameTime * 0.95 + (endFrameTime - beginFrameTime) * 0.05; //calculating the average
		}

		return localFrameTime;
	}

	void VulkanRenderDevice::EndTimestamp(void* commandBufferHandle) {
		uint32_t queryIndex = m_QueryStack.top() + 1;
		vkCmdWriteTimestamp((VkCommandBuffer)commandBufferHandle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_QueryPools[Renderer::GetCurrentFrameIndex()], queryIndex);
		m_QueryStack.pop();
	}

	void VulkanRenderDevice::Destroy() {
		for (uint32_t i = 0; i < m_QueryPools.size(); i++)
			vkDestroyQueryPool(VulkanContextDevice::Get().GetLogicalDevice(), m_QueryPools[i], nullptr);
		RenderDevice::Destroy();
	}

	void VulkanRenderDevice::SubmitImmediateCommand(std::function<void(VkCommandBuffer)>&& func) {
		const auto& queue = m_CommandQueue.As<VulkanCommandQueue>();
		VkCommandBuffer commandBuffer = queue->BeginSingleTimeCommand();
		func(commandBuffer);
		queue->EndSingleTimeCommand();
	}
}