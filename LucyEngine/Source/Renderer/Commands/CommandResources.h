#pragma once

#include "Core/Base.h"

#include "../Mesh.h"
#include "Renderer/Context/GraphicsPipeline.h"

namespace Lucy {

	//Priority for the upcoming job system (multithreading)
	enum class Priority : uint8_t {
		HIGH, LOW
	};

	using CommandResourceHandle = uint64_t;

	class CommandResource {
	public:
		CommandResource(Ref<GraphicsPipeline> pipeline, CommandFunc&& CommandFunc, bool isChild = false);
		CommandResource() = default;
		~CommandResource() = default;

		inline const Ref<GraphicsPipeline>& GetTargetPipeline() const { return m_Pipeline; }
		inline const CommandFunc& GetCommandFunc() const { return m_CommandFunc; }
	private:
		static CommandResourceHandle CreateUniqueHandle();

		void DoPass(void* commandBuffer);
		void EnqueueCommand(Ref<RenderCommand> renderCommand);

		Ref<GraphicsPipeline> m_Pipeline = nullptr;
		CommandFunc m_CommandFunc;
		std::vector<Ref<RenderCommand>> m_RenderCommandList;

		bool m_IsChild = false;
		std::vector<CommandResourceHandle> m_ChildCommandResourceHandles;

		friend class CommandQueue;
		friend class VulkanCommandQueue;
	};

	class RenderCommand {
	public:
		RenderCommand(Priority priority);
		virtual ~RenderCommand() = default;

		virtual void DoPass(void* commandBuffer, CommandResource* component) = 0;

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

		void DoPass(void* commandBuffer, CommandResource* component) final override;

		Ref<Mesh> Mesh = nullptr;
		glm::mat4 EntityTransform;
	};

	struct CubeRenderCommand final : public RenderCommand {
		CubeRenderCommand(VkImageView image, VkImageLayout layout, VkSampler sampler, Ref<Mesh> cubeMesh);
		virtual ~CubeRenderCommand() = default;

		void DoPass(void* commandBuffer, CommandResource* component) final override;

		Ref<Mesh> CubeMesh;
		VkImageView ImageView;
		VkImageLayout Layout;
		VkSampler Sampler;
	};

	//helper render command
	struct ImGuiRenderCommand final : public RenderCommand {
		ImGuiRenderCommand();
		virtual ~ImGuiRenderCommand() = default;

		void DoPass(void* commandBuffer, CommandResource* component) final override;
	};
}