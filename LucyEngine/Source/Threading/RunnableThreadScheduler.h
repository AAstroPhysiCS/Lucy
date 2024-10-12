#pragma once

#include <array>

#include "RunnableThread.h"

namespace Lucy {

	template <typename ... TRunnableThreads>
	concept IsRunnableThread = (std::is_base_of_v<RunnableThread, TRunnableThreads> && ...);

	template <typename ... TRunnableThreads> requires IsRunnableThread<TRunnableThreads...>
	class RunnableThreadScheduler final {
	public:
		RunnableThreadScheduler(const std::array<RunnableThreadCreateInfo, sizeof...(TRunnableThreads)>& runnableThreadArgs)
			: m_RunnableThreadArgs(std::move(runnableThreadArgs)) {
		}

		~RunnableThreadScheduler() {
			m_Running = false;

			while (m_ThreadFinishCount.load() != m_RunnableThreads.size()) { /* Wait for the threads to finish */ }

			for (auto& thread : m_ThreadPool) {
				LUCY_ASSERT(thread.joinable(), "Faulty thread join!");
				thread.join();
			}
		}

		RunnableThreadScheduler(const RunnableThreadScheduler& other) = delete;
		RunnableThreadScheduler(RunnableThreadScheduler&& other) = delete;
		RunnableThreadScheduler& operator=(const RunnableThreadScheduler& other) = delete;
		RunnableThreadScheduler& operator=(RunnableThreadScheduler&& other) = delete;

		constexpr auto GetThreadExtent() const { return sizeof...(TRunnableThreads); }

		void Start() {
			m_Running = true;
			const auto CreateThread = [&](const auto& runnableThread, uint32_t threadIndex) {
#ifdef LUCY_WINDOWS
				std::thread& thread = m_ThreadPool.emplace_back([&runnableThread, threadIndex, this]() {
					LUCY_ASSERT(runnableThread->OnInit(threadIndex), "Thread: {0} failed to initialize!", runnableThread->GetDebugName());

					uint32_t statusCode = runnableThread->OnRun();
					LUCY_ASSERT(statusCode, "Invalid status code on thread {0}", runnableThread->GetDebugName());

					runnableThread->OnJoin();
					m_ThreadFinishCount.fetch_add(1);
				});

				runnableThread->SetID(thread.get_id());
				
				HANDLE handle = thread.native_handle();
				uint32_t affinity = runnableThread->GetAffinity();

				SetThreadArguments(handle, runnableThread->GetDebugName(),
								   affinity == ThreadApplicationAffinityIncremental ? 1uLL << threadIndex : affinity,
								   ConvertToPlatformSpecificPrioritySystem(runnableThread->GetPriority()));

				m_RunnableThreads.emplace_back(runnableThread);
			};
#endif

			size_t i = 0;
			(CreateThread(Memory::CreateRef<TRunnableThreads>(m_RunnableThreadArgs[i++]), i), ...);
		}

		template <typename TRunnableThread> requires IsRunnableThread<TRunnableThread>
		inline Ref<TRunnableThread> GetThread() {
			if (!m_Running) //this means that it is being run in singlethreaded policy
				return nullptr;

			for (const auto& runnableThread : m_RunnableThreads) {
				if (auto casted = runnableThread->As<TRunnableThread>(); casted)
					return casted;
			}
			LUCY_ASSERT("Thread cannot be found!");
			return nullptr;
		}
	private:
		std::vector<Ref<RunnableThread>> m_RunnableThreads;
		std::vector<std::thread> m_ThreadPool;

		std::atomic_int m_ThreadFinishCount;

		std::array<RunnableThreadCreateInfo, sizeof...(TRunnableThreads)> m_RunnableThreadArgs;

		bool m_Running = false;
	};
}