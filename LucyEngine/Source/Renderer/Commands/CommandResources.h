#pragma once

#include "Core/Base.h"

#include "../Mesh.h"
#include "Renderer/Context/ContextPipeline.h"

namespace Lucy {

	//Priority for the upcoming job system (multithreading)
	enum class Priority : uint8_t {
		HIGH, LOW
	};

	using CommandResourceHandle = uint64_t;

	class CommandResource {
	public:
		CommandResource(Ref<ContextPipeline> pipeline, CommandFunc&& CommandFunc, bool isChild = false);
		CommandResource() = default;
		~CommandResource() = default;

		inline const Ref<ContextPipeline>& GetTargetPipeline() const { return m_Pipeline; }
		inline const CommandFunc& GetCommandFunc() const { return m_CommandFunc; }

		inline bool IsChildValid() const {
			if (m_IsChild && m_ChildCommandResourceHandles.size() != 0) {
				LUCY_CRITICAL("Child resources should not have child resources!");
				LUCY_ASSERT(false);
			}
			return m_IsChild;
		}
	private:
		static CommandResourceHandle CreateUniqueHandle();

		void DoPass(void* commandBuffer);
		void EnqueueCommand(Ref<RenderCommand> renderCommand);

		Ref<ContextPipeline> m_Pipeline = nullptr;
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

	struct ComputeDispatchCommand final : public RenderCommand {
		ComputeDispatchCommand(Priority priority, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		virtual ~ComputeDispatchCommand() = default;

		void DoPass(void* commandBuffer, CommandResource* component) final override;

		inline const uint32_t GetGroupCountX() const { return m_GroupCountX; }
		inline const uint32_t GetGroupCountY() const { return m_GroupCountY; }
		inline const uint32_t GetGroupCountZ() const { return m_GroupCountZ; }
	private:
		uint32_t m_GroupCountX = 0;
		uint32_t m_GroupCountY = 0;
		uint32_t m_GroupCountZ = 0;
	};

	//helper render command
	struct ImGuiRenderCommand final : public RenderCommand {
		ImGuiRenderCommand();
		virtual ~ImGuiRenderCommand() = default;

		void DoPass(void* commandBuffer, CommandResource* component) final override;
	};
}