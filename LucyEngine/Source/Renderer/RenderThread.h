#pragma once

#include <future>

#include "Threading/RunnableThread.h"

#include "Renderer/RendererConfiguration.h"

namespace Lucy {

	class Window;
	class RenderCommandQueue;

	class RendererBackend;

	struct RenderThreadCreateInfo {
		Ref<Window> Window = nullptr;
		RendererConfiguration Config;
		std::promise<void>& InitPromise;
	};

	class RenderThread final : public RunnableThread {
	public:
		RenderThread(const RunnableThreadCreateInfo& createInfo, const RenderThreadCreateInfo& renderThreadCreateInfo);
		virtual ~RenderThread() = default;

		RenderThread(const RenderThread& other) = delete;
		RenderThread(RenderThread&& other) noexcept = delete;
		RenderThread& operator=(const RenderThread& other) = delete;
		RenderThread& operator=(RenderThread&& other) noexcept = delete;
	public:
		inline bool IsOnRenderThread() const { return GetID() == std::this_thread::get_id(); }

		void SignalToShutdown();
		void WaitToShutdown();

		const Ref<RendererBackend>& GetBackend() const;
	private:
		bool OnInit() final override;
		uint32_t OnRun() final override;
		void OnJoin() final override;

		bool m_Running = false;
		bool m_Finished = false;
		std::condition_variable m_FinishedCondVar;

		std::thread m_ThreadNative;

		RenderThreadCreateInfo m_RenderThreadCreateInfo;
		Ref<RendererBackend> m_Backend = nullptr;
	};
}
