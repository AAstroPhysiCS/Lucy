#include "lypch.h"
#include "RenderThread.h"

#include "Core/Window.h"
#include "Events/EventHandler.h"

#include "Renderer.h"

namespace Lucy {

	RenderThread::RenderThread(const RunnableThreadCreateInfo& createInfo, const RenderThreadCreateInfo& renderThreadCreateInfo) 
		: RunnableThread(createInfo), m_RenderThreadCreateInfo(renderThreadCreateInfo) {
	}

	void RenderThread::SignalToShutdown() {
		LUCY_ASSERT(!IsOnRenderThread(), "SignalToShutdown should only be called from the main thread!");
		m_Running = false;
	}

	void RenderThread::WaitToShutdown() {
		LUCY_ASSERT(!IsOnRenderThread(), "WaitToShutdown should only be called from the main thread!");
		static std::mutex m;

		std::unique_lock lock(m);
		m_FinishedCondVar.wait(lock, [&]() { return m_Finished.load(); });
	}

	bool RenderThread::OnInit() {
		LUCY_PROFILE_NEW_EVENT("RenderThread::OnInit");
		LUCY_ASSERT(IsOnRenderThread(), "OnInit function is being called on a another thread!");
		LUCY_INFO("Renderer running on separate thread");

		tracy::SetThreadName(GetDebugName().c_str());

		m_Backend = RendererBackend::Create(m_RenderThreadCreateInfo.Config, m_RenderThreadCreateInfo.Window);
		Renderer::RTSetBackend(m_Backend);
		m_Backend->Init();

		m_RenderThreadCreateInfo.InitPromise.set_value();
		m_Running = true;
		return m_Running;
	}

	uint32_t RenderThread::OnRun() {
		LUCY_ASSERT(IsOnRenderThread(), "OnRun function is being called on a another thread!");

		m_Backend->FlushCommandQueue();
		
		while (m_Running) {
			LUCY_PROFILE_NEW_EVENT("RenderThread::OnRun");

			if (Application::IsMainThreadReady()) {
				RenderContextResultCodes result = Renderer::WaitAndPresent();
				if (result == RenderContextResultCodes::ERROR_OUT_OF_DATE_KHR ||
					result == RenderContextResultCodes::SUBOPTIMAL_KHR ||
					result == RenderContextResultCodes::NOT_READY) {
					EventHandler::DispatchImmediateEvent<SwapChainResizeEvent>();
				}

				Application::SetMainThreadReady(false);
				Application::IsMainThreadReadyCondVar().notify_one();
			}
		}
		return true;
	}

	void RenderThread::OnJoin() {
		LUCY_PROFILE_NEW_EVENT("RenderThread::OnJoin");
		LUCY_ASSERT(IsOnRenderThread(), "OnJoin function is being called on a another thread!");
		m_Backend->Destroy();
		m_Finished = true;
		m_FinishedCondVar.notify_one();
	}
}