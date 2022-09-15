#include "lypch.h"
#include "CommandResources.h"

#include "Utils/UUID.h"

namespace Lucy {

	CommandResourceHandle RenderCommandResource::CreateUniqueHandle() {
		static IDProvider m_IDProvider;
		return m_IDProvider.RequestID();
	}

	RenderCommandResource::RenderCommandResource(CommandFunc&& CommandFunc, Ref<GraphicsPipeline> pipeline)
		: m_Pipeline(pipeline), m_CommandFunc(CommandFunc) {
	}

	void RenderCommandResource::EnqueueCommand(Ref<RenderCommand> renderCommand) {
		switch (renderCommand->GetPriority()) {
			case Priority::HIGH:
				m_RenderCommandList.insert(m_RenderCommandList.begin(), renderCommand);
				break;
			case Priority::LOW:
				m_RenderCommandList.push_back(renderCommand);
				break;
		}
	}

	void RenderCommandResource::DoPass(void* commandBuffer) {
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

	void StaticMeshRenderCommand::DoPass(void* commandBuffer, RenderCommandResource* component) {
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

	void ImGuiRenderCommand::DoPass(void* commandBuffer, Lucy::RenderCommandResource* component) {
		component->GetCommandFunc()(commandBuffer, nullptr, this);
	}

	InternalRenderCommand::InternalRenderCommand(VkImageView imageView, VkImageLayout layout, VkSampler sampler, Ref<Mesh> cubeMesh)
		: RenderCommand(Priority::HIGH), ImageView(imageView), Layout(layout), Sampler(sampler), CubeMesh(cubeMesh) {
#ifdef LUCY_DEBUG
		m_Name = "InternalRenderCommand";
#endif
	}

	void InternalRenderCommand::DoPass(void* commandBuffer, RenderCommandResource* component) {
		component->GetCommandFunc()(commandBuffer, component->GetTargetPipeline(), this);
	}
}