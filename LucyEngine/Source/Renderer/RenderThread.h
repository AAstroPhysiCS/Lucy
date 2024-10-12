#pragma once

#include <mutex>
#include <condition_variable>

#include "Threading/RunnableThread.h"

namespace Lucy {

	class Window;
	class CommandQueue;

	class RenderThread final : public RunnableThread {
	public:
		RenderThread(const RunnableThreadCreateInfo& createInfo);
		virtual ~RenderThread() = default;

		bool OnInit(uint32_t threadIndex) final override;
		uint32_t OnRun() final override;
		void OnJoin() final override;
		void OnStop() final override;
	public:
		void SubmitCommandQueue(Ref<CommandQueue> commandQueue);
		void SignalToPresent();
		bool IsOnRenderThread() const;
		
		inline RenderContextResultCodes GetResultCode() const { return m_LastFrameResultCode; }
	private:
		void ExecuteCommands();

		bool m_Running = false;

		Ref<CommandQueue> m_CommandQueue = nullptr;

		std::condition_variable m_ExecutionVariable;
		static inline std::mutex m_ExecutionMutex;
		RenderContextResultCodes m_LastFrameResultCode = RenderContextResultCodes::SUCCESS;
	};
}
