#pragma once

#include "Context/RendererAPI.h"
#include "Context/VulkanContext.h"

#include "Renderer/VulkanCommandPool.h"
#include "Context/VulkanPipeline.h"
#include "VulkanRenderPass.h"
#include "Synchronization/SynchItems.h"

namespace Lucy {

	class VulkanRenderer : public RendererAPI {
	private:
		static uint32_t s_ImageIndex;
	public:
		inline static uint32_t CURRENT_FRAME = 0;
		inline static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

		VulkanRenderer(RenderArchitecture renderArchitecture);
		virtual ~VulkanRenderer() = default;

		void Init() override;

		void ClearCommands();
		void Execute();
		void Present();
		void Destroy();
		void Dispatch();

		void BeginScene(Scene& scene);
		void EndScene();

		void DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size);
		void Submit(const Func&& func);
		void SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform);

		void OnFramebufferResize(float sizeX, float sizeY);
		Entity OnMousePicking();
	private:
		RefLucy<VulkanPipeline> m_GeometryPipeline;

		std::vector<Semaphore> m_ImageIsAvailableSemaphores;
		std::vector<Semaphore> m_RenderIsFinishedSemaphores;
		std::vector<Fence> m_InFlightFences;
		std::vector<Fence*> m_ImagesInFlight;

		static RefLucy<VulkanCommandPool> s_CommandPool;
	};
}