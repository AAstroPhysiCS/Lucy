#include "lypch.h"
#include "RenderThread.h"

#include "Core/Window.h"

#include "Renderer.h"

namespace Lucy {

	//Run function runs every single iteration. But first we must ensure that the RenderThread kicks off at the right time
	//Since the main thread and the render thread needs to run simultaneously, we have to keep some sort of synchronization between these two.
	//Hence the conditional variable.

	RenderThread::RenderThread(const RunnableThreadCreateInfo& createInfo)
		: RunnableThread(createInfo) {
	}

	bool RenderThread::OnInit(uint32_t threadIndex) {
		LUCY_ASSERT(IsOnRenderThread(), "OnInit function is being called on a another thread!");
		LUCY_INFO("Renderer running on separate thread");
		return true;
	}

	uint32_t RenderThread::OnRun() {
		LUCY_ASSERT(IsOnRenderThread(), "OnRun function is being called on a another thread!");
		while (m_Running) {
			std::unique_lock<std::mutex> lock;
			m_ExecutionVariable.wait(lock, [this]() { return m_Running; });
			ExecuteCommands();
		}
		return true;
	}

	void RenderThread::OnJoin() {
		LUCY_ASSERT(IsOnRenderThread(), "OnJoin function is being called on a another thread!");
		m_CommandQueue->Free();
		m_Running = false;
	}

	void RenderThread::OnStop() {
		LUCY_ASSERT(IsOnRenderThread(), "OnStop function is being called on a another thread!");
		LUCY_INFO("Render thread is shutting down!");
	}

	void RenderThread::ExecuteCommands() {
		LUCY_PROFILE_NEW_EVENT("RenderThread::ExecuteCommands");
		m_CommandQueue->FlushCommandQueue();
		//m_CommandQueue->FlushSubmitQueue();
	}

	void RenderThread::SubmitCommandQueue(Ref<CommandQueue> commandQueue) {
		m_CommandQueue = commandQueue;
	}

	void RenderThread::SignalToPresent() {
		LUCY_ASSERT(!IsOnRenderThread(), "SignalToPresent function should be called from the Game thread!");
		m_ExecutionVariable.notify_one();
	}

	bool RenderThread::IsOnRenderThread() const {
		return GetID() == std::this_thread::get_id();
	}
}