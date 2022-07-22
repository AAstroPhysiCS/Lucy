#pragma once

#include "RenderDevice.h"

namespace Lucy {

	struct ImGuiPipeline;

	class VulkanRenderDevice final : public RenderDevice {
	public:
		VulkanRenderDevice(RenderArchitecture arch);
		~VulkanRenderDevice() = default;

		void Init() override;
		void Destroy() override;

		void BeginScene(Scene& scene) override;
		void RenderScene() override;
		PresentResult EndScene() override;
		void Wait();
	public:
		void BindBuffers(void* commandBufferHandle, Ref<Mesh> mesh);
		void BindPushConstant(void* commandBufferHandle, Ref<Pipeline> pipeline, const PushConstant& pushConstant);
		void BindPipeline(void* commandBufferHandle, Ref<Pipeline> pipeline); //for CommandQueue
		void BindDescriptorSet(void* commandBufferHandle, Ref<Pipeline> pipeline, Ref<DescriptorSet> descriptorSet);
		//commandBufferHandle is the handle of the commandBuffer
		//Vulkan: VkCommandBuffer
		void BindBuffers(void* commandBufferHandle, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer);
		void DrawIndexed(void* commandBufferHandle, uint32_t indexCount, uint32_t instanceCount,
								 uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

		void BeginRenderPass(void* commandBufferHandle, Ref<Pipeline> pipeline);
		void EndRenderPass(Ref<Pipeline> pipeline);
	public:
		static void RecordSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func);
		void DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size);

		void OnWindowResize() override;
		void OnViewportResize() override;
		Entity OnMousePicking() override;

		void UIPass(const ImGuiPipeline& imguiPipeline, std::function<void(VkCommandBuffer commandBuffer)>&& imguiRenderFunc); //vulkan only
	};
}