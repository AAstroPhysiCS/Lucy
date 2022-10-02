#include "lypch.h"
#include "CommandResources.h"

#include "Utils/UUID.h"

namespace Lucy {

	CommandResourceHandle CommandResource::CreateUniqueHandle() {
		static IDProvider m_IDProvider;
		return m_IDProvider.RequestID();
	}

	CommandResource::CommandResource(Ref<ContextPipeline> pipeline, CommandFunc&& CommandFunc, bool isChild)
		: m_Pipeline(pipeline), m_CommandFunc(CommandFunc), m_IsChild(isChild) {
	}

	void CommandResource::EnqueueCommand(Ref<RenderCommand> renderCommand) {
		switch (renderCommand->GetPriority()) {
			case Priority::HIGH:
				m_RenderCommandList.insert(m_RenderCommandList.begin(), renderCommand);
				break;
			case Priority::LOW:
				m_RenderCommandList.push_back(renderCommand);
				break;
		}
	}

	void CommandResource::DoPass(void* commandBuffer) {
		for (Ref<RenderCommand> renderCommand : m_RenderCommandList)
			renderCommand->DoPass(commandBuffer, this);
		m_RenderCommandList.clear();
	}

	RenderCommand::RenderCommand(Priority priority)
		: m_Priority(priority) {
	}

	StaticMeshRenderCommand::StaticMeshRenderCommand(Priority priority, Ref<Lucy::Mesh> mesh, const glm::mat4& entityTransform)
		: RenderCommand(priority), Mesh(mesh), EntityTransform(entityTransform) {
#ifdef LUCY_DEBUG
		m_Name = "StaticMeshRenderCommand";
#endif
	}

	void StaticMeshRenderCommand::DoPass(void* commandBuffer, CommandResource* component) {
		if (!Mesh)
			return;
		component->GetCommandFunc()(commandBuffer, component->GetTargetPipeline(), this);
	}

	ImGuiRenderCommand::ImGuiRenderCommand()
		: RenderCommand(Priority::LOW) {
#ifdef LUCY_DEBUG
		m_Name = "ImGuiRenderCommand";
#endif
	}

	void ImGuiRenderCommand::DoPass(void* commandBuffer, Lucy::CommandResource* component) {
		component->GetCommandFunc()(commandBuffer, nullptr, this);
	}

	CubeRenderCommand::CubeRenderCommand(VkImageView imageView, VkImageLayout layout, VkSampler sampler, Ref<Mesh> cubeMesh)
		: RenderCommand(Priority::HIGH), ImageView(imageView), Layout(layout), Sampler(sampler), CubeMesh(cubeMesh) {
#ifdef LUCY_DEBUG
		m_Name = "CubeRenderCommand";
#endif
	}

	void CubeRenderCommand::DoPass(void* commandBuffer, CommandResource* component) {
		component->GetCommandFunc()(commandBuffer, component->GetTargetPipeline(), this);
	}

	ComputeDispatchCommand::ComputeDispatchCommand(Priority priority, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
		: RenderCommand(priority), m_GroupCountX(groupCountX), m_GroupCountY(groupCountY), m_GroupCountZ(groupCountZ) {
	}

	void ComputeDispatchCommand::DoPass(void* commandBuffer, CommandResource* component) {
		if (m_GroupCountX == 0 || m_GroupCountY == 0 || m_GroupCountZ == 0) {
			LUCY_CRITICAL("Dispatching compute cannot be processed, since the group count is 0.");
			LUCY_ASSERT(false);
		}
		component->GetCommandFunc()(commandBuffer, component->GetTargetPipeline(), this);
	}
}