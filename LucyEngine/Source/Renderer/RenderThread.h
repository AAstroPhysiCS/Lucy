#pragma once

#include <future>

#include "Threading/RunnableThread.h"

namespace Lucy {

	class Window;
	class RenderCommandQueue;

	struct RenderThreadCreateInfo {
		Ref<Window> Window = nullptr;
		RendererConfiguration Config;
		std::promise<void>& InitPromise;
	};

	class RenderThread final : public RunnableThread {
	public:
		RenderThread(const RunnableThreadCreateInfo& createInfo, const RenderThreadCreateInfo& renderThreadCreateInfo);
		virtual ~RenderThread() = default;
	public:
		inline bool IsOnRenderThread() const { return GetID() == std::this_thread::get_id(); }

		void SignalToShutdown();
		void WaitToShutdown();

		inline const Ref<RendererBackend>& GetBackend() const { return m_Backend; }
	private:
		bool OnInit() final override;
		uint32_t OnRun() final override;
		void OnJoin() final override;

		std::atomic_bool m_Running = false;

		std::atomic_bool m_Finished = false;
		std::condition_variable m_FinishedCondVar;

		std::thread m_ThreadNative;

		RenderThreadCreateInfo m_RenderThreadCreateInfo;
		Ref<RendererBackend> m_Backend = nullptr;
	};
}
