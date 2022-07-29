#pragma once

#include "Renderer/Commands/CommandQueue.h"

namespace Lucy {

	class RenderDeviceCommandList {
	public:
		virtual ~RenderDeviceCommandList() = default;

		static Ref<RenderDeviceCommandList> Create();

		inline const Ref<CommandQueue>& GetCommandQueue() const { return m_CommandQueue; }
	protected:
		virtual void Init() = 0;
		virtual void ExecuteCommandQueue() = 0;

		void DispatchCommands();
		void Recreate();
		void Free();

		void EnqueueToRenderThread(EnqueueFunc&& func);
		RenderCommandResourceHandle CreateRenderPassResource(RenderCommandFunc&& func, Ref<Pipeline> pipeline);

		template <typename Command, typename ... Args>
		inline void EnqueueRenderCommand(RenderCommandResourceHandle resourceHandle, Args&&... args) {
			m_CommandQueue->EnqueueRenderCommand(resourceHandle, Memory::CreateRef<Command>(args...));
		}
		
		std::vector<EnqueueFunc> m_RenderFunctionQueue;
		Ref<CommandQueue> m_CommandQueue = nullptr;

		RenderDeviceCommandList() = default;

		friend class RenderDevice;
		friend class VulkanRenderDevice;
	};
}

