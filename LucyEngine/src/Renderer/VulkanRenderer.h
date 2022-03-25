#pragma once

#include "Context/RendererAPI.h"
#include "Context/VulkanContext.h"

#include "Renderer/VulkanCommandPool.h"
#include "Context/VulkanPipeline.h"
#include "VulkanRenderPass.h"
#include "Synchronization/SynchItems.h"

namespace Lucy {

	struct ImDrawData;

	class VulkanRenderer : public RendererAPI {
	private:
		static uint32_t s_ImageIndex;
	public:
		inline static uint32_t CURRENT_FRAME = 0;
		inline static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

		VulkanRenderer(RenderArchitecture renderArchitecture);
		virtual ~VulkanRenderer() = default;

		void Init() override;

		void ClearCommands() override;
		void Execute() override;
		void Present();
		void Destroy() override;
		void Dispatch() override;

		void BeginScene(Scene& scene) override;
		void EndScene() override;

		void DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size);
		void Submit(const Func&& func) override;
		void SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform) override;
		static void RecordSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func);

		void OnFramebufferResize(float sizeX, float sizeY) override;
		Entity OnMousePicking() override;
	private:
		static RefLucy<VulkanPipeline> m_GeometryPipeline;

		std::vector<Semaphore> m_ImageIsAvailableSemaphores;
		std::vector<Semaphore> m_RenderIsFinishedSemaphores;
		std::vector<Fence> m_InFlightFences;
		std::vector<Fence*> m_ImagesInFlight;

		static RefLucy<VulkanCommandPool> s_CommandPool;
	private:
		friend class ImGuiLayer;
	};
}