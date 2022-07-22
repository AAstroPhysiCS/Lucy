#include "lypch.h"
#include "RenderDevice.h"

#include "Renderer/VulkanRenderDevice.h"

namespace Lucy {

	Ref<RenderDevice> RenderDevice::Create(RenderArchitecture arch) {
		switch (arch) {
			case RenderArchitecture::Vulkan:
				return Memory::CreateRef<VulkanRenderDevice>(arch);
				break;
		}
		return nullptr;
	}
	
	RenderDevice::RenderDevice(RenderArchitecture renderArchitecture) {
		m_Architecture = renderArchitecture;
		s_CommandQueue = CommandQueue::Create();
	}

	void RenderDevice::Dispatch() {
		uint32_t oldSize = m_RenderFunctionQueue.size();
		for (uint32_t i = 0; i < oldSize; i++) {
			m_RenderFunctionQueue[i](); //functions can contain nested functions
		}
		//meaing that nested lambda functions are being run in the second iteration.
		m_RenderFunctionQueue.erase(m_RenderFunctionQueue.begin(), m_RenderFunctionQueue.begin() + oldSize);
	}

	void RenderDevice::ClearQueues() {
		m_StaticMeshDrawCommands.clear();
		s_CommandQueue->Clear();
	}

	void RenderDevice::Enqueue(EnqueueFunc&& func) {
		m_RenderFunctionQueue.push_back(func);
	}

	void RenderDevice::EnqueueStaticMesh(Priority priority, Ref<Mesh> mesh, const glm::mat4& entityTransform) {
		switch (priority) {
			case Priority::HIGH:
				m_StaticMeshDrawCommands.insert(m_StaticMeshDrawCommands.begin(), Memory::CreateRef<MeshDrawCommand>(priority, mesh, entityTransform));
				break;
			case Priority::LOW:
				m_StaticMeshDrawCommands.push_back(Memory::CreateRef<MeshDrawCommand>(priority, mesh, entityTransform));
				break;
		}
	}

	void RenderDevice::RecordStaticMeshToCommandQueue(Ref<Pipeline> pipeline, RecordFunc<VkCommandBuffer, Ref<DrawCommand>>&& func) {
		CommandElement element;
		element.Pipeline = pipeline;
		element.RecordFunc = *(RecordFunc<void*, Ref<DrawCommand>>*) &func;
		element.Arguments = m_StaticMeshDrawCommands;

		s_CommandQueue->Enqueue(element);
	}
}
