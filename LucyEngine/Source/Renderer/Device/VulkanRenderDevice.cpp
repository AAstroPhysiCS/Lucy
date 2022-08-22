#include "lypch.h"
#include "VulkanRenderDevice.h"

#include "Renderer/Context/VulkanPipeline.h"
#include "Renderer/Descriptors/VulkanDescriptorSet.h"
#include "Renderer/Memory/Buffer/Vulkan/VulkanFrameBuffer.h"

#include "Renderer/Renderer.h"

#include "VulkanRenderDeviceCommandList.h"

#include "../Mesh.h"

namespace Lucy {

	void VulkanRenderDevice::BindBuffers(void* commandBufferHandle, Ref<Mesh> mesh) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindBuffers");
		
		VertexBindInfo vertexInfo;
		vertexInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		mesh->GetVertexBuffer()->Bind(vertexInfo);

		IndexBindInfo indexInfo;
		indexInfo.CommandBuffer = vertexInfo.CommandBuffer;
		mesh->GetIndexBuffer()->Bind(indexInfo);
	}

	void VulkanRenderDevice::BindPushConstant(void* commandBufferHandle, Ref<Pipeline> pipeline, const PushConstant& pushConstant) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindPushConstant");
		
		PushConstantBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		bindInfo.PipelineLayout = pipeline.As<VulkanPipeline>()->GetPipelineLayout();
		pushConstant.Bind(bindInfo);
	}

	void VulkanRenderDevice::BindPipeline(void* commandBufferHandle, Ref<Pipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindPipeline");

		PipelineBindInfo info;
		info.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		info.CommandBuffer = (VkCommandBuffer)commandBufferHandle;

		pipeline->Bind(info);
	}

	void VulkanRenderDevice::UpdateDescriptorSets(Ref<Pipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::UpdateDescriptorSets");

		for (Ref<DescriptorSet> descriptorSet : pipeline->GetDescriptorSets()) {
			Ref<VulkanDescriptorSet> vulkanSet = descriptorSet.As<VulkanDescriptorSet>();
			vulkanSet->Update();
		}
	}

	void VulkanRenderDevice::BindAllDescriptorSets(void* commandBufferHandle, Ref<Pipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindAllDescriptorSets");

		Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();

		VulkanDescriptorSetBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		bindInfo.PipelineLayout = vulkanPipeline->GetPipelineLayout();

		for (Ref<DescriptorSet> descriptorSet : pipeline->GetDescriptorSets()) {
			Ref<VulkanDescriptorSet> vulkanSet = descriptorSet.As<VulkanDescriptorSet>();
			vulkanSet->Bind(bindInfo);
		}
	}

	void VulkanRenderDevice::BindDescriptorSet(void* commandBufferHandle, Ref<Pipeline> pipeline, uint32_t setIndex) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindDescriptorSet");

		Ref<VulkanPipeline> vulkanPipeline = pipeline.As<VulkanPipeline>();
		
		VulkanDescriptorSetBindInfo bindInfo;
		bindInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		bindInfo.PipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		bindInfo.PipelineLayout = vulkanPipeline->GetPipelineLayout();

		for (Ref<DescriptorSet> descriptorSet : pipeline->GetDescriptorSets()) {
			if (descriptorSet->GetSetIndex() == setIndex) {
				Ref<VulkanDescriptorSet> vulkanSet = descriptorSet.As<VulkanDescriptorSet>();
				vulkanSet->Bind(bindInfo);
				break;
			}
		}
	}

	void VulkanRenderDevice::BindBuffers(void* commandBufferHandle, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BindBuffers");

		VertexBindInfo vertexInfo;
		vertexInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		vertexBuffer->Bind(vertexInfo);

		IndexBindInfo indexInfo;
		indexInfo.CommandBuffer = vertexInfo.CommandBuffer;
		indexBuffer->Bind(indexInfo);
	}

	void VulkanRenderDevice::DrawIndexed(void* commandBufferHandle, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
		vkCmdDrawIndexed((VkCommandBuffer)commandBufferHandle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void VulkanRenderDevice::BeginRenderPass(void* commandBufferHandle, Ref<Pipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::BeginRenderPass");

		Ref<RenderPass> renderPass = pipeline->GetRenderPass();
		Ref<FrameBuffer> frameBuffer = pipeline->GetFrameBuffer();

		RenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.CommandBuffer = (VkCommandBuffer)commandBufferHandle;
		renderPassBeginInfo.Width = frameBuffer->GetWidth();
		renderPassBeginInfo.Height = frameBuffer->GetHeight();

		renderPassBeginInfo.VulkanFrameBuffer = frameBuffer.As<VulkanFrameBuffer>()->GetVulkanHandles()[Renderer::GetCurrentFrameIndex()];

		renderPass->Begin(renderPassBeginInfo);
	}

	void VulkanRenderDevice::EndRenderPass(Ref<Pipeline> pipeline) {
		LUCY_PROFILE_NEW_EVENT("VulkanRenderDevice::EndRenderPass");

		pipeline->GetRenderPass()->End();
	}

	void VulkanRenderDevice::ExecuteSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func) {
		m_RenderDeviceCommandList.As<VulkanRenderDeviceCommandList>()->ExecuteSingleTimeCommand(std::move(func));
	}
}