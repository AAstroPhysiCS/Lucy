#include "lypch.h"
#include "RunnableThread.h"

namespace Lucy {

	RunnableThread::RunnableThread(const RunnableThreadCreateInfo& createInfo)
		: m_DebugName(createInfo.Name), m_Affinity(createInfo.Affinity), m_Priority(createInfo.Priority) {
	}

	void RunnableThread::Start() {
		const auto CreateThread = [&]() {
#ifdef LUCY_WINDOWS
			m_ThreadNative = std::thread([this]() {
				LUCY_ASSERT(OnInit(), "Thread: {0} failed to initialize!", GetDebugName());

				uint32_t statusCode = OnRun();
				LUCY_ASSERT(statusCode, "Invalid status code on thread {0}", GetDebugName());

				OnJoin();
			});

			SetID(m_ThreadNative.get_id());

			HANDLE handle = m_ThreadNative.native_handle();
			uint32_t affinity = GetAffinity();

			SetThreadArguments(handle, GetDebugName(),
				affinity == ThreadApplicationAffinityIncremental ? 1uLL << 1 : affinity,
				ConvertToPlatformSpecificPrioritySystem(GetPriority()));
		};
#endif
		CreateThread();
	}
}
