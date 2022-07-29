#pragma once

#include "Core/Base.h"

#include "../Mesh.h"
#include "Renderer/Context/Pipeline.h"

namespace Lucy {

	//Priority for the upcoming job system (multithreading)
	enum class Priority : uint8_t {
		HIGH, LOW
	};

	using RenderCommandResourceHandle = uint64_t;

	class RenderCommandResource {
	public:
		static RenderCommandResourceHandle CreateUniqueHandle();

		RenderCommandResource(RenderCommandFunc&& renderCommandFunc, Ref<Pipeline> pipeline);
		RenderCommandResource() = default;
		~RenderCommandResource() = default;

		inline const Ref<Pipeline>& GetTargetPipeline() const { return m_Pipeline; }
		inline const RenderCommandFunc& GetRenderCommandFunc() const { return m_RenderCommandFunc; }
	private:
		void DoPass(void* commandBuffer);
		void EnqueueRenderCommand(Ref<RenderCommand> renderCommand);

		Ref<Pipeline> m_Pipeline = nullptr;
		RenderCommandFunc m_RenderCommandFunc;
		std::vector<Ref<RenderCommand>> m_RenderCommandList;

		friend class CommandQueue;
		friend class VulkanCommandQueue;
	};

	class RenderCommand {
	public:
		RenderCommand(Priority priority);
		virtual ~RenderCommand() = default;

		virtual void DoPass(void* commandBuffer, RenderCommandResource* component) = 0;

		inline Priority GetPriority() { return m_Priority; }
	protected:
#ifdef LUCY_DEBUG
		std::string m_Name = "Unknown"; //for debugging purposes
#endif
		Priority m_Priority = Priority::LOW; //start with low
	};

	struct StaticMeshRenderCommand final : public RenderCommand {
		StaticMeshRenderCommand(Priority priority, Ref<Mesh> mesh, const glm::mat4& entityTransform);
		virtual ~StaticMeshRenderCommand() = default;

		void DoPass(void* commandBuffer, RenderCommandResource* component) final override;

		Ref<Mesh> Mesh = nullptr;
		glm::mat4 EntityTransform;
	};

	//helper render command
	struct ImGuiRenderCommand final : public RenderCommand {
		ImGuiRenderCommand();
		virtual ~ImGuiRenderCommand() = default;

		void DoPass(void* commandBuffer, RenderCommandResource* component) final override;
	};
}