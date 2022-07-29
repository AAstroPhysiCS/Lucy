#include "lypch.h"
#include "RenderCommandResources.h"

#include "Utils/UUID.h"

namespace Lucy {
	
	RenderCommandResourceHandle RenderCommandResource::CreateUniqueHandle() {
		static IDProvider m_IDProvider;
		return m_IDProvider.RequestID();
	}

	RenderCommandResource::RenderCommandResource(RenderCommandFunc&& renderCommandFunc, Ref<Pipeline> pipeline)
		: m_Pipeline(pipeline), m_RenderCommandFunc(renderCommandFunc) {
	}

	void RenderCommandResource::EnqueueRenderCommand(Ref<RenderCommand> renderCommand) {
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
		m_Name = "MeshRenderCommand";
#endif
	}

	void StaticMeshRenderCommand::DoPass(void* commandBuffer, RenderCommandResource* component) {
		if (!Mesh)
			return;
		component->GetRenderCommandFunc()(commandBuffer, component->GetTargetPipeline(), this);
	}

	ImGuiRenderCommand::ImGuiRenderCommand()
		: RenderCommand(Priority::LOW) {
#ifdef LUCY_DEBUG
		m_Name = "ImGuiRenderCommand";
#endif
	}

	void ImGuiRenderCommand::DoPass(void* commandBuffer, Lucy::RenderCommandResource* component) {
		component->GetRenderCommandFunc()(commandBuffer, nullptr, this);
	}
}