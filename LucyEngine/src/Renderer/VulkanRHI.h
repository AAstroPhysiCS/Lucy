#pragma once

#include "Context/RHI.h"
#include "Context/VulkanContext.h"

#include "Synchronization/SynchItems.h"

#include "VulkanRenderPass.h"

namespace Lucy {

	struct ImDrawData;

	class VulkanRHI : public RHI {
	public:
		VulkanRHI(RenderArchitecture renderArchitecture);
		virtual ~VulkanRHI() = default;

		void Init() override;

		void Dispatch() override;
		void ClearCommands() override;
		void Destroy() override;

		void BeginScene(Scene& scene) override;
		PresentResult RenderScene() override;
		void EndScene() override;

		void DirectCopyBuffer(VkBuffer& stagingBuffer, VkBuffer& buffer, VkDeviceSize size);
		void Submit(const Func&& func) override;
		void SubmitMesh(RefLucy<Pipeline> pipeline, RefLucy<Mesh> mesh, const glm::mat4& entityTransform) override;
		void SubmitRenderCommand(const RenderCommand& renderCommand) override;

		static void RecordSingleTimeCommand(std::function<void(VkCommandBuffer)>&& func);
		
		void OnViewportResize() override;
		Entity OnMousePicking() override;
	};
}