#pragma once

#include "Context/RendererAPI.h"
#include "Context/VulkanContext.h"

#include "Context/VulkanPipeline.h"

namespace Lucy {

	class VulkanRenderer : public RendererAPI {
	private:
		inline static const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
		inline static uint32_t CURRENT_FRAME = 0;
	public:
		VulkanRenderer(RenderArchitecture renderArchitecture);
		virtual ~VulkanRenderer() = default;

		void Init() override;

		void ClearCommands();
		void Draw();
		void Presentation();
		void Destroy();
		void Dispatch();

		void BeginScene(Scene& scene);
		void EndScene();

		void Submit(const Func&& func);
		void SubmitMesh(RefLucy<Mesh> mesh, const glm::mat4& entityTransform);

		void OnFramebufferResize(float sizeX, float sizeY);
		Entity OnMousePicking();

	private:
		RefLucy<VulkanPipeline> m_GeometryPipeline;

		std::vector<VkSemaphore> m_ImageIsAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderIsFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		std::vector<VkFence> m_ImagesInFlight;

		static uint32_t s_ImageIndex;
	};
}